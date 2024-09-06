/*
 * CS 261 PA4: Mini-ELF interpreter
 *
 * Name: Cameron Hakenson
 */

#include "p4-interp.h"
y86_reg_t easyOPQ(y86_reg_t *valA, y86_reg_t valB, y86_t *cpu, y86_inst_t inst);

/**********************************************************************
 *                         REQUIRED FUNCTIONS
 *********************************************************************/

y86_reg_t decode_execute (y86_t *cpu, y86_inst_t inst, bool *cnd, y86_reg_t *valA)
{
    /* check for null params */
    if (cpu == NULL
        || cnd == NULL
        || valA == NULL) {
            cpu->stat = INS;
            return 0;
        }

    /* check for valid program counter */
    if (cpu->pc >= MEMSIZE || cpu->pc < 0) {
        cpu->stat = ADR;
        return 0;
    }

    y86_reg_t *regArray = cpu->reg;
    y86_reg_t valE = 0;
    y86_reg_t valB;

    /* decode and execute */
    switch(inst.icode) {
        case HALT:
            cpu->stat = HLT;
            break;
        case NOP: cpu->pc = inst.valP; break;
        case IRMOVQ: valE = inst.valC.v; break;
        case OPQ:
            *valA = regArray[inst.ra];
            valB = regArray[inst.rb];
            valE = easyOPQ(valA, valB, cpu, inst);
            break;
        case PUSHQ:
            *valA = regArray[inst.ra];
            valB = regArray[RSP];
            valE = valB - 8;
            break;
        case POPQ:
            *valA = regArray[RSP];
            valB = regArray[RSP];
            valE = valB + 8;
            break;
        case RMMOVQ:
            *valA = regArray[inst.ra];
            valB = regArray[inst.rb];
            valE = valB + inst.valC.d;
            break;
        case MRMOVQ:
            valB = regArray[inst.rb];
            valE = valB + inst.valC.d;
            break;
        case CALL:
            valB = regArray[RSP];
            valE = valB - 8;
            break;
        case RET:
            *valA = regArray[RSP];
            valB = regArray[RSP];
            valE = valB + 8;
            break;
        default: break;
    }

    return valE;
}

void memory_wb_pc (y86_t *cpu, y86_inst_t inst, byte_t *memory,
        bool cnd, y86_reg_t valA, y86_reg_t valE)
{
    y86_reg_t *regArray = cpu->reg;
    address_t *mem;
    y86_reg_t valM;

    /* memory and wb */
    switch(inst.icode) {
        case HALT: cpu->pc = inst.valP; break;
        case NOP: cpu->pc = inst.valP; break;
        case IRMOVQ:
            regArray[inst.rb] = valE;
            cpu->pc = inst.valP;
            break;
        case OPQ:
            regArray[inst.rb] = valE;
            cpu->pc = inst.valP;
            break;
        case PUSHQ:
            mem = (uint64_t *) (&memory[valE]);
            *mem = valA;
            regArray[RSP] = valE;
            cpu->pc = inst.valP;
            break;
        case POPQ:
            mem = (uint64_t *) (&memory[valA]);
            valM = *mem;
            regArray[RSP] = valE;
            regArray[inst.ra] = valM;
            cpu->pc = inst.valP;
            break;
        case RMMOVQ:
            mem = (uint64_t *) (&memory[valE]);
            *mem = valA;
            cpu->pc = inst.valP;
            break;
        case MRMOVQ:
            if (valE >= MEMSIZE) {
                cpu->stat = ADR;
                break;
            }
            mem = (uint64_t *) (&memory[valE]);
            valM = *mem;
            regArray[inst.ra] = valM;
            cpu->pc = inst.valP;
            break;
        case CALL:
            mem = (uint64_t *) (&memory[valE]);
            *mem = inst.valP;
            regArray[RSP] = valE;
            cpu->pc = inst.valC.dest;
            break;
        case RET:
            mem = (uint64_t *) (&memory[valA]);
            valM = *mem;
            regArray[RSP] = valE;
            cpu->pc = valM;
            break;
        default: break;
    }

}
/**********************************************************************
 *                         HELPER FUNCTIONS
 *********************************************************************/

/* a helper function that makes the opq operations cleaner */
y86_reg_t easyOPQ(y86_reg_t *valA, y86_reg_t valB, y86_t *cpu, y86_inst_t inst) {

    y86_reg_t valE = 0;

    switch(inst.ifun.op) {
        case ADD:
            valE = *valA + valB;
            if ((*valA < 0 && valB < 0 && valE > 0) || (*valA > 0 && valB > 0 && valE < 0)) {
                cpu->of = true;
            }
            break;
        case SUB:
            valE = valB - *valA;
            if ((*valA > 0 && valB < 0 && valE > 0) || (*valA < 0 && valB > 0 && valE < 0)) {
                cpu->of = true;
            }
            break;
        case AND: valE = valB & *valA; break;
        case XOR: valE = *valA ^ valB; break;
        case BADOP: cpu->stat = INS; return valE;
    }

    /* sets zero of sign flags if needed */
    if (valE == 0) {
        cpu->zf = true;
    } else if (valE < 0) {
        cpu->sf = true;
    }
    return valE;
}

/**********************************************************************
 *                         OPTIONAL FUNCTIONS
 *********************************************************************/

void usage_p4 (char **argv)
{
    printf("Usage: %s <option(s)> mini-elf-file\n", argv[0]);
    printf(" Options are:\n");
    printf("  -h      Display usage\n");
    printf("  -H      Show the Mini-ELF header\n");
    printf("  -a      Show all with brief memory\n");
    printf("  -f      Show all with full memory\n");
    printf("  -s      Show the program headers\n");
    printf("  -m      Show the memory contents (brief)\n");
    printf("  -M      Show the memory contents (full)\n");
    printf("  -d      Disassemble code contents\n");
    printf("  -D      Disassemble data contents\n");
    printf("  -e      Execute program\n");
    printf("  -E      Execute program (trace mode)\n");
}

bool parse_command_line_p4 (int argc, char **argv,
        bool *print_header, bool *print_phdrs,
        bool *print_membrief, bool *print_memfull,
        bool *disas_code, bool *disas_data,
        bool *exec_normal, bool *exec_debug, char **filename)
{
    // checks for valid parameters
    if (argv == NULL 
            || print_header == NULL
            || print_phdrs == NULL
            || print_membrief == NULL
            || print_memfull == NULL 
            || disas_code == NULL
            || disas_data == NULL
            || exec_normal == NULL
            || exec_debug == NULL
            || filename == NULL) {
        return false;
    }
 
    // parameter parsing w/ getopt()
    int c;
    while ((c = getopt(argc, argv, "hHafsmMdDeE")) != -1) {
        switch (c) {
            case 'h': usage_p4(argv); return true;
            case 'H': *print_header = true; break;
            case 'a':
                if (*print_memfull) {
                    usage_p4(argv);
                    return false;
                }
                *print_header = true;
                *print_phdrs = true;
                *print_membrief = true;
                break;
            case 'f':
                if (*print_membrief) {
                    usage_p4(argv);
                    return false;
                }
                *print_header = true;
                *print_phdrs = true;
                *print_memfull = true;
                break;
            case 's': *print_phdrs = true; break;
            case 'm':
                if (*print_memfull) {
                    usage_p4(argv);
                    return false;
                }
                *print_membrief = true;
                break;
            case 'M':
                if (*print_membrief) {
                    usage_p4(argv);
                    return false;
                }
                *print_memfull = true;
                break;
            case 'd': *disas_code = true; break;
            case 'D': *disas_data = true; break;
            case 'e':
                if (*exec_debug) {
                    usage_p4(argv);
                    return false;
                }
                *exec_normal = true;
                break;
            case 'E':
                if (*exec_normal) {
                    usage_p4(argv);
                    return false;
                }
                *exec_debug = true;
                break;
            default: usage_p4(argv); return false;
        }
    }
 
    if (optind != argc-1) {
        // no filename (or extraneous input)
        usage_p4(argv);
        return false;
    }
    *filename = argv[optind];   // save filename

    return true;
}

void dump_cpu_state (y86_t *cpu)
{

    y86_reg_t *regArray = cpu->reg;

    /* prints cpu state*/
    printf("Y86 CPU state:\n");
    printf("    PC: %016lx   flags: Z%d S%d O%d     ",  cpu->pc, cpu->zf, cpu->sf, cpu->of);

    /* switch for status codes */
    switch(cpu->stat) {
        case(AOK): printf("AOK\n"); break;
        case(HLT): printf("HLT\n"); break;
        case(ADR): printf("ADR\n"); break;
        case(INS): printf("INS\n"); break;
    }

    printf("  %%rax: %016lx    %%rcx: %016lx\n", regArray[RAX], regArray[RCX]);
    printf("  %%rdx: %016lx    %%rbx: %016lx\n", regArray[RDX], regArray[RBX]);
    printf("  %%rsp: %016lx    %%rbp: %016lx\n", regArray[RSP], regArray[RBP]);
    printf("  %%rsi: %016lx    %%rdi: %016lx\n", regArray[RSI], regArray[RDI]);
    printf("   %%r8: %016lx     %%r9: %016lx\n", regArray[R8], regArray[R9]);
    printf("  %%r10: %016lx    %%r11: %016lx\n", regArray[R10], regArray[R11]);
    printf("  %%r12: %016lx    %%r13: %016lx\n", regArray[R12], regArray[R13]);
    printf("  %%r14: %016lx", regArray[R14]);

}

