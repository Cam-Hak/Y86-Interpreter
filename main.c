/*
 * CS 261: Main driver
 *
 * Name: Cameron Hakenson
 */

#include "p1-check.h"
#include "p2-load.h"
#include "p3-disas.h"
#include "p4-interp.h"

int main (int argc, char **argv)
{
    // parse command-line options
    bool print_header = false;
    bool print_phdrs = false;
    bool print_membrief = false;
    bool print_memfull = false;
    bool disas_code = false;
    bool disas_data = false;
    bool exec_normal = false;
    bool exec_debug = false;

    char *fn = NULL;
    if (!parse_command_line_p4(argc, argv, &print_header,
    &print_phdrs, &print_membrief, &print_memfull, &disas_code, &disas_data, &exec_normal, &exec_debug, &fn)) {
        exit(EXIT_FAILURE);
    }

    if (fn != NULL) {
 
        // open Mini-ELF binary
        FILE *f = fopen(fn, "r");
        if (!f) {
            printf("Failed to read file\n");
            exit(EXIT_FAILURE);
        }
 
        // P1: load and check Mini-ELF header
        elf_hdr_t hdr;
        if (!read_header(f, &hdr)) {
            printf("Failed to read file\n");
            exit(EXIT_FAILURE);
        }
 
        // P1 output
        if (print_header) {
            dump_header(&hdr);
        }

        // initiates program header array
        elf_phdr_t phdr[hdr.e_num_phdr];
        // dumps program header information with read_phdr
        for (int i = 0; i < hdr.e_num_phdr; i++) {
            if (read_phdr(f, hdr.e_phdr_start + (i * sizeof(elf_phdr_t)), &phdr[i]) == false) {
                exit(EXIT_FAILURE);
            }
        }

        // initiates memory array with all zero values
        byte_t* memory = (byte_t*)calloc(MEMSIZE, 1);
        for (int i = 0; i < hdr.e_num_phdr; i++) {
            if (load_segment(f, memory, &phdr[i]) == false) {
                exit(EXIT_FAILURE);
            }
        }

        // prints program header information if valid flag was passed
        if (print_phdrs) {
            dump_phdrs(hdr.e_num_phdr, phdr);
        }

        // prints a section of memory if valid flag was passed
        if (print_membrief) {
            for (int i = 0; i < hdr.e_num_phdr; i++) {
                dump_memory(memory, phdr[i].p_vaddr, phdr[i].p_vaddr + phdr[i].p_size);
            }
        }

        // prints the contents of the memory array if valid flag was passed
        if (print_memfull) {
            dump_memory(memory, 0, MEMSIZE);
        }

        if (disas_code) {
            printf("Disassembly of executable contents:\n");
            for (int i = 0; i < hdr.e_num_phdr; i++) {
                if (phdr[i].p_type == CODE) {
                    disassemble_code(memory, &phdr[i], &hdr);
                    printf("\n");
                }
            }
        }

        if (disas_data) {
            printf("Disassembly of data contents:\n");
            for (int i = 0; i < hdr.e_num_phdr; i++) {
                if (phdr[i].p_flags == 6 && phdr[i].p_type == DATA) {
                    disassemble_data(memory, &phdr[i]);
                    printf("\n");
                } else if (phdr[i].p_flags == 4 && phdr[i].p_type == DATA) {
                    disassemble_rodata(memory, &phdr[i]);
                    printf("\n");
                }
            }
        }

        /* p4 */

        y86_t cpu;
        /* setting cpu contents to zero */
        memset(&cpu, 0, sizeof(cpu));

        cpu.pc = hdr.e_entry;
        cpu.stat = AOK;
        int exec_count = 0;
        bool cnd = false;
        y86_reg_t valA = 0;
        y86_reg_t valE = 0;

        /* normal execution */
        if (exec_normal) {

            printf("Beginning execution at 0x%04x\n", hdr.e_entry);

            while(cpu.stat == AOK) {

                /* fetch instruction */
                y86_inst_t ins = fetch(&cpu, memory);
                cnd = false;

                /* decode and execute the fetched instruction */
                valE = decode_execute(&cpu, ins, &cnd, &valA);

                /* write values to registers and memory if needed */
                memory_wb_pc(&cpu, ins, memory, cnd, valA, valE);

                /* increment the execution counter */
                exec_count++;

                /* stop infinite loop bug */
                if (cpu.pc >= MEMSIZE) {
                    cpu.stat = ADR;
                }
            }

            /* prints out register information */
            dump_cpu_state(&cpu);
            printf("\nTotal execution count: %d\n", exec_count);

        }

        /* debug execution */
        if (exec_debug) {

            printf("Beginning execution at 0x%04x\n", hdr.e_entry);

            /* dump initial state of the cpu*/
            dump_cpu_state(&cpu);

            while(cpu.stat == AOK) {

                y86_inst_t ins = fetch(&cpu, memory);
                cnd = false;

                /* call disasseble function to display the instruction */
                printf("\n\nExecuting: ");
                disassemble(&ins);
                printf("\n");

                valE = decode_execute(&cpu, ins, &cnd, &valA);

                memory_wb_pc(&cpu, ins, memory, cnd, valA, valE);
                /* dump cpu state after a memory write back */
                dump_cpu_state(&cpu);

                exec_count++;

                if (cpu.pc >= MEMSIZE) {
                    cpu.stat = ADR;
                }
            }

            printf("\nTotal execution count: %d\n\n", exec_count);
            
            /* dump the entire contents of memory */
            dump_memory(memory, 0, MEMSIZE);

        }
        

        // cleanup
        free(memory);
        fclose(f);
 
    }

    return EXIT_SUCCESS;
}

