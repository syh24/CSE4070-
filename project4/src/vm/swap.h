#ifndef SWAP_H
#define SWAP_H

#include <hash.h>
#include <bitmap.h>
#include <list.h>
#include "devices/block.h"
#include "vm/page.h"
#include "vm/frame.h"

struct bitmap *bitmap;

void swap_init();
void swap_in(size_t used_index, void *kaddr);
size_t swap_out(void *kaddr);

#endif