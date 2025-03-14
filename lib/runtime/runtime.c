#include <stdio.h>
#include <inttypes.h>
#include "stdlib.h"

// Function declarations
extern uint64_t lisp_entry(void *heap);
void lisp_error(char *exp);

#define num_mask   0b11
#define num_tag    0b00
#define num_shift  2

#define bool_mask  0b1111111
#define bool_tag   0b0011111
#define bool_shift 7

#define heap_mask  0b111

#define pair_tag   0b010
#define vector_tag 0b101
#define nil_val    0b11111111

uint64_t print_value(uint64_t value) {
    if ((value & num_mask) == num_tag) {
        int64_t ivalue = (int64_t)value;
        printf("%" PRIi64, ivalue >> num_shift);
    } else if ((value & bool_mask) == bool_tag) {
        printf("%s", (value >> bool_shift) ? "true" : "false");
    } else if (value == nil_val) {
        printf("()");
    } else if ((value & heap_mask) == pair_tag) {
        uint64_t* pair_ptr = (uint64_t*)(value - pair_tag);
        printf("(pair ");
        print_value(pair_ptr[0]);
        printf(" ");
        print_value(pair_ptr[1]);
        printf(")");
    } else if ((value & heap_mask) == vector_tag) {
        uint64_t* vec_ptr = (uint64_t*)(value - vector_tag);
        uint64_t length = vec_ptr[0];
        printf("[");
        for (uint64_t i = 0; i < length; i++) {
            if (i > 0) printf(" ");
            print_value(vec_ptr[i + 1]);
        }
        printf("]");
    } else {
        printf("BAD VALUE: %" PRIu64, value);
    }
    return value;
}

void ensure_num(uint64_t val) {
    if ((val & num_mask) != num_tag) {
        lisp_error("expected number");
    }
}

void ensure_pair(uint64_t val) {
    if ((val & heap_mask) != pair_tag) {
        lisp_error("expected pair");
    }
}

void ensure_vector(uint64_t val) {
    if ((val & heap_mask) != vector_tag) {
        lisp_error("expected vector");
    }
}

void ensure_vector_index(uint64_t vec, uint64_t idx) {
    ensure_vector(vec);
    uint64_t* vec_ptr = (uint64_t*)(vec - vector_tag);
    uint64_t length = vec_ptr[0];
    if ((idx & num_mask) != num_tag) {
        lisp_error("vector index must be a number");
    }
    idx = idx >> num_shift;
    if (idx < 0 || idx >= length) {
        lisp_error("vector index out of bounds");
    }
}

void lisp_error(char* exp) {
    printf("Stuck[%s]", exp);
    exit(1);
}

int main(int argc, char** argv) {
    void* heap = malloc(4096);
    print_value(lisp_entry(heap));
    return 0;
}