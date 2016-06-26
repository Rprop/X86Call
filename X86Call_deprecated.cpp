#include <RLib_Import.h>

//-------------------------------------------------------------------------

#pragma pack(1)
typedef struct X86_STUB
{
	char  op_rsp[3]; // rsp
	DWORD addr_rsp;
	char  op_rdi[3]; // rdi
	DWORD addr_rdi;
	char  op_rbp[3]; // rbp
	DWORD addr_rbp;

	// x86 start
	char op_call_1[5];  // call next line
	char op_mov_23h[8]; // mov dword ptr [rsp+4],23h
	char op_add_10h[7]; // add dword ptr [rsp],10h
	char op_retf_1[1];  // retf

	// x86 code
	char op_esp[1];     // mov esp, 11111111h
	DWORD addr_esp;
	char op_ebp[2];     // mov ebp, esp
	char op_nop[0x40];  // dynamicly generated code
	char op_eax_rev[3]; // mov dword ptr ss:[esp], eax

	// x86 end
	char op_push_33h[2]; // push 33h
	char op_call_2[5];   // call next line
	char op_add_5[4];    // add dword ptr[rsp], 5
	char op_retf_2[1];   // retf

	char  op_rax_rev[3]; // rax rev
	DWORD addr_rax_rev;
	char  op_rbp_rev[3]; // rbp rev
	DWORD addr_rbp_rev;
	char  op_rdi_rev[3]; // rdi rev
	DWORD addr_rdi_rev;
	char  op_rsp_rev[3]; // rsp rev
	DWORD addr_rsp_rev;

	char op_ret; // ret

public:
	typedef struct X86_STACK
	{
		DWORD64 rax; // x86 return value
		DWORD64 rbp;
		DWORD64 rdi;
		DWORD64 rsp;
	} *PX86_STACK;

	X86_STUB *reloc(DWORD64 esp) {
		assert(this != nullptr);
		auto stack = reinterpret_cast<PX86_STACK>(esp - sizeof(X86_STACK));

		this->addr_rsp = static_cast<DWORD>(reinterpret_cast<DWORD64>(&stack->rsp) - reinterpret_cast<DWORD64>(&this->op_rdi));
		this->addr_rdi = static_cast<DWORD>(reinterpret_cast<DWORD64>(&stack->rdi) - reinterpret_cast<DWORD64>(&this->op_rbp));
		this->addr_rbp = static_cast<DWORD>(reinterpret_cast<DWORD64>(&stack->rbp) - reinterpret_cast<DWORD64>(&this->op_call_1));
		this->addr_rax_rev = static_cast<DWORD>(reinterpret_cast<DWORD64>(&stack->rax) - reinterpret_cast<DWORD64>(&this->op_rbp_rev));
		this->addr_rbp_rev = static_cast<DWORD>(reinterpret_cast<DWORD64>(&stack->rbp) - reinterpret_cast<DWORD64>(&this->op_rdi_rev));
		this->addr_rdi_rev = static_cast<DWORD>(reinterpret_cast<DWORD64>(&stack->rdi) - reinterpret_cast<DWORD64>(&this->op_rsp_rev));
		this->addr_rsp_rev = static_cast<DWORD>(reinterpret_cast<DWORD64>(&stack->rsp) - reinterpret_cast<DWORD64>(&this->op_ret));
		this->addr_esp = static_cast<DWORD>(esp - sizeof(X86_STACK));
		return this;
	}
} *PX86_STUB;
#pragma pack()

//-------------------------------------------------------------------------

static PX86_STUB AllocateX86Stack(_Out_ DWORD64 *stackBase, _In_opt_ DWORD64 sp = 0x10000ULL, _In_opt_ DWORD64 size = 0x10000ULL)
{
	if (sp == 0) goto __alloc_random_address;

	while (sp < 0xFFFFFFFFULL) {
		MEMORY_BASIC_INFORMATION m;
		VirtualQuery(reinterpret_cast<LPVOID>(sp), &m, sizeof(m));
		if (m.State != MEM_FREE) {
			// 64kb aligned
			if (m.AllocationBase && m.RegionSize) {
				sp = reinterpret_cast<DWORD64>(m.AllocationBase) + Utility::round_up(m.RegionSize, 0x10000ULL);
			} else {
				sp += 0x10000ULL;
			} //if		
			
			continue;
		} //if

__alloc_random_address:
		auto p = VirtualAlloc(reinterpret_cast<LPVOID>(sp), size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (p != nullptr) {
			*stackBase = reinterpret_cast<DWORD64>(p) + size;
			return reinterpret_cast<PX86_STUB>(p);
		} //if
	} //for

	return nullptr;
}

//-------------------------------------------------------------------------

extern "C" LPVOID  _x86call();
extern "C" DWORD64 _rsp();

//-------------------------------------------------------------------------

extern "C" DWORD32 __cdecl X86Call(DWORD64 func, bool stdcall, int argc, ...)
{
	PX86_STUB x86;
	DWORD64 stack;
	BOOL isWow64;
	IsWow64Process(GetCurrentProcess(), &isWow64);
	if (isWow64) {
		x86   = AllocateX86Stack(&stack, 0, sizeof(*x86));
		stack = _rsp();
	} else {
		x86 = AllocateX86Stack(&stack);
	} //if

	memcpy(x86, _x86call(), sizeof(*x86));
	x86->reloc(stack);

	auto nops = x86->op_nop;
	auto insc = argc * 5;
	auto espc = static_cast<char>(argc * sizeof(DWORD32));

	va_list args;
	va_start(args, argc);
	while (--argc >= 0) {
		nops[argc * 5] = '\x68'; // x86 push
		*reinterpret_cast<DWORD32 *>(&nops[argc * 5 + 1]) = va_arg(args, DWORD32);
	}
	va_end(args);
	
	//nops[insc++] = '\xCC';

	nops[insc] = '\xE8'; // x86 call
	*reinterpret_cast<DWORD32 *>(&nops[insc + 1]) = static_cast<DWORD32>(func - reinterpret_cast<DWORD64>(&nops[insc + 5]));
	insc += 5;
	
	if (!stdcall){
		nops[insc]     = '\x83'; // add esp
		nops[insc + 1] = '\xC4';
		nops[insc + 2] = espc;
		insc += 3;
	} //if

	assert(insc < _countof(x86->op_nop));
	if (_countof(x86->op_nop) - insc >= 6){
		nops[insc]     = '\xEB'; // skip nop, jmp
		nops[insc + 1] = static_cast<char>(x86->op_eax_rev - &nops[insc + 2]);
		//insc += 2;
	} //if

	auto ret = reinterpret_cast<decltype(&_x86call)>(x86)();
	if (x86) {
		VirtualFree(x86, 0, MEM_RELEASE);
	} //if
	return static_cast<DWORD32>(reinterpret_cast<DWORD64>(ret));
}

//-------------------------------------------------------------------------

extern "C" __declspec(dllexport) DWORD64 __stdcall X86CallTest(DWORD64 x86_func)
{
	return X86Call(x86_func, false, 3, 2, 4, 6);
}

//-------------------------------------------------------------------------

int WINAPI DllMain( _In_ HINSTANCE, _In_ DWORD, _In_ LPVOID)
{
	return TRUE;
}
