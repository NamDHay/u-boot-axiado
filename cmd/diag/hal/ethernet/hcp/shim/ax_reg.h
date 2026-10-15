#ifndef __REGS_H__
#define __REGS_H__

#define CLEAR_32BIT 0x0
#define FILL_32BIT 0xFFFFFFFF

#include <stdint.h>
#include <asm/io.h>
#include <dm.h>
#include <stdio.h>
// #define BIT(n) 		(1UL << (n))
#define BIT64(_n) 	(1ULL << (_n))
#define UNSET(x)	~(1U << (x))
#define UNSET64(x)	~(1ULL << (x))

#define ALIGN_OFFSET(value, align) ((value) & ((align) - 1))
// #define IS_ALIGNED(value, align) (ALIGN_OFFSET((value), (align)) == 0)


/**
* @brief Evaluates most significant half word from word
* 
*/
#define MSHW(x) ((u16)(((x) >> 16) & 0xFFFF))
 
/**
* @brief Evaluates least significant half word from word
* 
*/
#define LSHW(x) ((u16)((x)&0xFFFF))
 
/**
* @brief Evaluates most significant byte from word
* 
*/
#define MSBM(x) ((u8)(((x) >> 24) & 0xFF))
 
/**
* @brief Evaluates second most significant byte from word
* 
*/
#define MSBL(x) ((u8)(((x) >> 16) & 0xFF))
 
/**
* @brief Evaluates second least significant byte from word
* 
*/
#define LSBM(x) ((u8)(((x) >> 8) & 0xFF))
 
/**
* @brief Evaluates least significant byte from word
* 
*/
#define LSBL(x) ((u8)((x)&0xFF))
 
/**
* @brief Extract the most significant word (32bit) from a 64bit value 
* 
*/
#define MSWD(x) (u32)((x) >> 32)
/**
* @brief Extract the least significant word (32bit) from a 64bit value 
* 
*/
#define LSWD(x) (u32)((x))

static inline void WRITE8(void *base, u32 offset, u8 value)
{
    // *(volatile u8 *)((u8 *)base + offset) = value;
	writeb(value, base + offset);
}

static inline u8 READ8(void *base, u32 offset)
{
    // return *(volatile u8 *)((u8 *)base + offset);
	return readb(base + offset);
}

static inline void WRITE16(void *base, u32 offset, u16 value)
{
    // *(volatile u16 *)((u8 *)base + offset) = value;
	writew(value, base + offset);
}

static inline u16 READ16(void *base, u32 offset)
{
    // return *(volatile u16 *)((u8 *)base + offset);
	return readw(base + offset);
}

static inline void WRITE32(void *base, u32 offset, u32 value)
{
    // *(volatile u32 *)((u8 *)base + offset) = value;
	writel(value, base + offset);
}

static inline u32 READ32(void *base, u32 offset)
{
    // return *(volatile u32 *)((u8 *)base + offset);
	return readl(base + offset);
}

static inline void WRITE64(void *base, u32 offset, u64 value)
{
    // *(volatile u64 *)((u8 *)base + offset) = value;
	writeq(value, base + offset);
}

static inline u64 READ64(void *base, u32 offset)
{
    // return *(volatile u64 *)((u8 *)base + offset);
	return readq(base + offset);
}

static inline void REG_WRITE8(u32 base_addr, u32 reg_offset,
			      u8 value)
{
	// *(volatile u8 *)(base_addr + reg_offset) = value;
	writeb(value, base_addr + reg_offset);
}

static inline u8 REG_READ8(u32 base_addr, u32 reg_offset)
{
	// return *(volatile u8 *)(base_addr + reg_offset);
	return readb(base_addr + reg_offset);
}

static inline void REG_WRITE16(u32 base_addr, u32 reg_offset,
			       u16 value)
{
	// *(volatile u16 *)(base_addr + reg_offset) = value;
	writew(value, base_addr + reg_offset);
}

static inline u16 REG_READ16(u32 base_addr, u32 reg_offset)
{
	// return *(volatile u16 *)(base_addr + reg_offset);
	return readw(base_addr + reg_offset);
}

static inline u32 REG_READ32(u32 base_addr, u32 reg_offset)
{
	// return *((volatile u32 *)(base_addr + reg_offset));
	return readl(base_addr + reg_offset);
}

static inline void REG_WRITE32(u32 base_addr, u32 reg_offset,
			       u32 value)
{
	// *((volatile u32 *)(base_addr + reg_offset)) = value;
	writel(value, base_addr + reg_offset);
}

static inline u64 REG_READ64(u32 reg, u64 offset)
{
	// return *((volatile u64 *)(reg + offset));
	return readq(reg + offset);
}

static inline void REG_WRITE64(u32 reg, u64 offset, u64 val)
{
	// *((volatile u64 *)(reg + offset)) = val;
	writeq(val, reg + offset);
}

static inline void SET_BIT32(u32 base, u32 offset, u32 bit_pos)
{
	REG_WRITE32(base, offset, REG_READ32(base, offset) | (1U << bit_pos));
}

static inline void CLEAR_BIT32(u32 base, u32 offset,
			       u32 bit_pos)
{
	REG_WRITE32(base, offset, REG_READ32(base, offset) & ~(1U << bit_pos));
}

static inline u32 GET_BIT32(u32 value, u32 bit_pos)
{
	return (value >> bit_pos) & 0x1U;
}

static inline u32 REG_READ32_DUMP(u32 base_addr, u32 reg_offset)
{
	// u32 data = *((volatile u32 *)(base_addr + reg_offset));
	u32 data = readl(base_addr + reg_offset);
	printf("Addr[0x%x]:0x%x\n", (u32)(base_addr + reg_offset), data);
	return data;
}

static inline void REG_WRITE32_DUMP(u32 base_addr, u32 reg_offset,
			       u32 value)
{
	printf("Addr[0x%x]:0x%x\n", (u32)(base_addr + reg_offset), value);
	// *((volatile u32 *)(base_addr + reg_offset)) = value;
	writel(value, base_addr + reg_offset);
}

static inline void REG_READ_MODIFY_WRITE32(u32 reg, u32 val)
{
    REG_WRITE32(reg, 0, (REG_READ32(reg, 0) | val));
}

static inline void REG_READ_MODIFY_WRITE64(u32 reg, u64 val)
{
    REG_WRITE64(reg, 0, (REG_READ64(reg, 0) | val));
}

static inline void REG_WRITE32_SPLIT(u32 base, u32 offset, u32 val)                                                         \
{                                                                                                      
	REG_WRITE32((base), ((offset) + 4), ((u32)(((u64)(val)) >> 32)));                  
	__asm volatile("dmb ishst");                                                                         
	REG_WRITE32((base), (offset), (u32)(val));                                              
	__asm volatile("dmb ishst");                                                                         
}

static inline u64 REG_READ32_SPLIT(u32 base, u32 offset)                                                               \
{                                                                                                    
	u32 val_lo, val_hi;                                                                             
	val_lo = REG_READ32(base, offset);                                                           
	val_hi = REG_READ32(base, offset + 4);                                                       
	return ((u64)val_lo | (u64)val_hi << 32);                                                         
}

static inline void REG_WRITE64_SPLIT(u32 base, u64 offset, u64 val)                                                         \
{                                                                                                      
	REG_WRITE64((base), ((offset) + 4), ((u64)(((u64)(val)) >> 32)));                  
	__asm volatile("dmb ishst");                                                                         
	REG_WRITE64((base), (offset), (u32)(val));                                              
	__asm volatile("dmb ishst");                                                                         
}

static inline u64 REG_READ64_SPLIT(u32 base, u64 offset)                                                               \
{                                                                                                    
	u32 val_lo, val_hi;                                                                             
	val_lo = REG_READ64(base, offset);                                                           
	val_hi = REG_READ64(base, offset + 4);                                                       
	return ((u64)val_lo | (u64)val_hi << 32);                                                         
}
/**
 * Below is function to handle bits from range [a,b] in register  
 */
static inline unsigned int read_bits(u32 register_value, u32 a, u32 b)
{
	if (a > b) {
		int temp = a;
		a = b;
		b = temp;
	}
	u64 mask = ((1U << (b - a + 1)) - 1) << a;
	u32 result = (register_value & mask) >> a;
	return result;
}

static inline unsigned int write_bits(u32 register_value, u32 new_value, int a,
			int b)
{
	if (a > b) {
		int temp = a;
		a = b;
		b = temp;
	}
	u64 mask = ((1ULL << (b - a + 1)) - 1ULL) << a;

	register_value &= ~mask;
	register_value |= (new_value << a) & mask;
	return register_value;
}

static inline void set(u32 addr, u32 pos, u32 pos_1, u32 value)
{
	unsigned int temp;
	temp = REG_READ32(addr, 0);
	temp = write_bits(temp, value, pos, pos_1);
	REG_WRITE32(addr, 0, temp);
}
static inline unsigned int get(u32 addr, u32 pos, u32 pos_1)
{
	unsigned int temp;
	temp = REG_READ32(addr, 0);
	temp = read_bits(temp, pos, pos_1);
	return temp;
}

static inline void clear(unsigned int addr, u32 pos, u32 pos_1)
{
	unsigned int temp;
	temp = REG_READ32(addr, 0);
	write_bits(temp, 0x0, pos, pos_1);
	REG_WRITE32(addr, 0, temp);
}

static inline int SetMem32(u32 base, u32 val, u32 SizeBytes)
{
	u32 i;

	for (i = 0; i < SizeBytes; i += 4) {
		REG_WRITE32(base, i, val + i);
	}

	printf("Set OK: %d bytes\n", SizeBytes);
	return 0;
}

static inline int CompareMem32(u32 Src, u32 Dst, u32 SizeBytes)
{
	u32 i;
	u32 ValSrc;
	u32 ValDst;

	for (i = 0; i < SizeBytes; i += 4) {
		ValSrc = REG_READ32(Src, i);
		ValDst = REG_READ32(Dst, i);

		if (ValSrc != ValDst) {
			printf("Mismatch @ +0x%X\n", i);
			printf("SRC[0x%08X] = 0x%08X\n", Src + i, ValSrc);
			printf("DST[0x%08X] = 0x%08X\n", Dst + i, ValDst);
			return 0;
		}
	}

	printf("Compare OK: %d bytes\n", SizeBytes);
	return 1;
}

#endif