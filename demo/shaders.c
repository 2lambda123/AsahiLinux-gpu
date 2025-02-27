#include <stdio.h>
#include "demo.h"

void agx_disassemble(void *_code, size_t maxlen, FILE *fp);

#define AGX_STOP \
	0x88, 0x00, 0x08, 0x00, 0x08, 0x00, 0x08, 0x00, 0x08, \
	0x00, 0x08, 0x00, 0x08, 0x00, 0x08, 0x00, 0x08, 0x00 \

#define AGX_BLEND \
	0x09, 0x00, 0x00, 0x04, 0xf0, 0xfc, 0x80, 0x03

/* Minimal vertex shader, where u4/u5 are preloaded by the paired compute
 * shader's uniform_store

   0: 9e03ca0202800100     imadd            $r0_r1, r5.discard, 32, u0
   8: 0e05c22218000000     iadd             r1, r1.discard, u1
  10: 0521000500c43200     device_load      1, 0, 0, 4, 0, i32, pair, r4_r5, r0_r1, 0, signed, lsl 1
  18: 0501104500c8f200     device_load      1, 1, 0, 4, 0, i32, quad, r0_r1_r2_r3, r0_r1, 1, signed, lsl 2
  20: 3800                 wait             0
  22: 622100000000         mov              r8, 0
  28: 1a99c8821800         fmul             r6, r4.discard, u4
  2e: 621d0000803f         mov              r7, 1065353216
  34: 1a91caa21800         fmul             r4, r5.discard, u5
  3a: 3801                 wait             1
  3c: 11208280             TODO.st_var      1, r8, 2
  40: 111c8380             TODO.st_var      1, r7, 3
  44: 11008480             TODO.st_var      1, r0, 4
  48: 11048580             TODO.st_var      1, r1, 5
  4c: 11088680             TODO.st_var      1, r2, 6
  50: 110c8780             TODO.st_var      1, r3, 7
  54: 11188080             TODO.st_var      1, r6, 0
  58: 91108180             TODO.st_var_final 1, r4, 1
*/

uint8_t vertex_shader[] = {
	0x9e, 0x03, 0xca, 0x02, 0x02, 0x80, 0x01, 0x00, 0x0e, 0x05, 0xc2, 0x22,
	0x18, 0x00, 0x00, 0x00, 0x05, 0x21, 0x00, 0x05, 0x00, 0xc4, 0x32, 0x00,
	0x05, 0x01, 0x10, 0x45, 0x00, 0xc8, 0xf2, 0x00, 0x38, 0x00, 0x62, 0x21,
	0x00, 0x00, 0x00, 0x00, 0x1a, 0x99, 0xc8, 0x82, 0x18, 0x00, 0x62, 0x1d,
	0x00, 0x00, 0x80, 0x3f, 0x1a, 0x91, 0xca, 0xa2, 0x18, 0x00, 0x38, 0x01,
	0x11, 0x20, 0x82, 0x80, 0x11, 0x1c, 0x83, 0x80, 0x11, 0x00, 0x84, 0x80,
	0x11, 0x04, 0x85, 0x80, 0x11, 0x08, 0x86, 0x80, 0x11, 0x0c, 0x87, 0x80,
	0x11, 0x18, 0x80, 0x80, 0x91, 0x10, 0x81, 0x80,

	AGX_STOP
};

/* Custom solid colour frag shader
   0: 6200873a             mov              r0l, 14983
   4: 62020531             mov              r0h, 12549
   8: 62040531             mov              r1l, 12549
   c: 6206003c             mov              r1h, 15360
  10: 4800c200             TODO.writeout    512, 3
  14: 480c0000             TODO.writeout    12, 0
  18: 09000004f0fc8003     TODO.blend       
*/

uint8_t fragment_shader[] = {
	0x48, 0x00, 0xc2, 0x00, 0x61, 0x01, 0x41, 0x80, 0x31, 0x80, 0x40, 0x00,
	0x00, 0x62, 0x0f, 0x00, 0x20, 0x00, 0x00, 0x00, 0x38, 0x00, 0x48, 0x0c,
	0x00, 0x00, 0x09, 0x00, 0x00, 0x04, 0xf0, 0xfc, 0x00, 0x03, 0x88, 0x00,
};

/*
  Compute shader implementing (float2) (1.0 / (dims * 0.5)), where dimensions
  is the ivec2 of width, height of the framebuffer (the address of which is
  preloadeded as u2_u3), since this shows up in our minimal vertex shader...
  I've seen Mali do this optimization before, but never so aggressively.

   0: 0501040d00c43200     device_load      1, 0, 0, 4, 0, i32, pair, r0_r1, u2_u3, 0, signed, lsl 1
   8: 3800                 wait             0
   a: be890a042c00         convert          u32_to_f, $r2, r0.discard, 1
  10: be810a242c00         convert          u32_to_f, $r0, r1.discard, 1
  16: 9a85c4020200         fmul             $r1, r2.discard, 0.5
  1c: 0a05c282             rcp              r1, r1.discard
  20: 9a81c0020200         fmul             $r0, r0.discard, 0.5
  26: 0a01c082             rcp              r0, r0.discard
  2a: c508803d00803000     uniform_store    2, i16, pair, 0, r1l_r1h, 8
  32: c500a03d00803000     uniform_store    2, i16, pair, 0, r0l_r0h, 10
  */

uint8_t vertex_pre[] = {
	0x05, 0x01, 0x04, 0x0d, 0x00, 0xc4, 0x32, 0x00,
	0x38, 0x00,
	0xbe, 0x89, 0x0a, 0x04, 0x2c, 0x00,
	0xbe, 0x81, 0x0a, 0x24, 0x2c, 0x00,
	0x9a, 0x85, 0xc4, 0x02, 0x02, 0x00,
	0x0a, 0x05, 0xc2, 0x82, 0x9a, 0x81, 0xc0, 0x02, 0x02, 0x00, 0x0a, 0x01,
	0xc0, 0x82, 0xc5, 0x08, 0x80, 0x3d, 0x00, 0x80, 0x30, 0x00, 0xc5, 0x00,
	0xa0, 0x3d, 0x00, 0x80, 0x30, 0x00,
	AGX_STOP
};

/* Clears the tilebuffer, where u6-u7 are preloaded with the FP16 clear colour
 * by the paired compute shader AUX2

   0: 7e018c098040         bitop_mov        r0, u6
   6: 7e058e098000         bitop_mov        r1, u7
   c: 09000004f0fc8003     TODO.blend
   */

uint8_t clear[] = {
	0x7e, 0x01, 0x8c, 0x09, 0x80, 0x40,
	0x7e, 0x05, 0x8e, 0x09, 0x80, 0x00,
	AGX_BLEND,
	AGX_STOP
};

uint8_t frag_aux3[] = {
	0x7e, 0x00, 0x04, 0x09, 0x80, 0x00,
	0xb1, 0x80, 0x00, 0x80, 0x00, 0x4a, 0x00, 0x00, 0x0a, 0x00,
	AGX_STOP
};

uint32_t
demo_upload_shader(const char *label, struct agx_allocator *allocator, uint8_t *code, size_t sz)
{
#if 0
	printf("%s:\n", label);
	agx_disassemble(code, sz, stdout);
	printf("\n");
#endif
	(void) label;

	return agx_upload(allocator, code, sz);
}

uint32_t
demo_vertex_shader(struct agx_allocator *allocator)
{
	FILE *fp = fopen("/Users/bloom/simple/shader_0.bin", "rb");
	fseek(fp, 0, SEEK_END);
	unsigned size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	struct agx_ptr ptr = agx_allocate(allocator, size);
	fread(ptr.map, 1, size, fp);
	fclose(fp);

	return ptr.gpu_va;
}

uint32_t
demo_fragment_shader(struct agx_allocator *allocator)
{
#if 0
	return demo_upload_shader("fragtex", allocator, fragment_shader, sizeof(fragment_shader));
#else
	FILE *fp = fopen("/Users/bloom/simple/shader_1.bin", "rb");
	fseek(fp, 0, SEEK_END);
	unsigned size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	struct agx_ptr ptr = agx_allocate(allocator, size);
	fread(ptr.map, 1, size, fp);
	fclose(fp);

	return ptr.gpu_va;
#endif
}

uint32_t
demo_vertex_pre(struct agx_allocator *allocator)
{
	return demo_upload_shader("vertex_pre", allocator, vertex_pre, sizeof(vertex_pre));
}

uint32_t
demo_clear(struct agx_allocator *allocator)
{
	return demo_upload_shader("clear", allocator, clear, sizeof(clear));
}

uint32_t
demo_frag_aux3(struct agx_allocator *allocator)
{
	return demo_upload_shader("frag_aux3", allocator, frag_aux3, sizeof(frag_aux3));
}
