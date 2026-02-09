#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x9f222e1e, "alloc_chrdev_region" },
	{ 0xa61fd7aa, "__check_object_size" },
	{ 0x092a35a2, "_copy_from_user" },
	{ 0xd710adbf, "__kmalloc_noprof" },
	{ 0xa53f4e29, "memcpy" },
	{ 0xcb8b6ec6, "kfree" },
	{ 0xd272d446, "__fentry__" },
	{ 0xe8213e80, "_printk" },
	{ 0xbd03ed67, "__ref_stack_chk_guard" },
	{ 0xd272d446, "__stack_chk_fail" },
	{ 0x90a48d82, "__ubsan_handle_out_of_bounds" },
	{ 0x8ea73856, "cdev_add" },
	{ 0x366ddfcc, "memchr" },
	{ 0xc1e6c71e, "__mutex_init" },
	{ 0x357aaab3, "mutex_lock_interruptible" },
	{ 0xe54e0a6b, "__fortify_panic" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0x092a35a2, "_copy_to_user" },
	{ 0x0bc5fb0d, "unregister_chrdev_region" },
	{ 0xf46d5bf3, "mutex_unlock" },
	{ 0x546c19d9, "validate_usercopy_range" },
	{ 0xe4de56b4, "__ubsan_handle_load_invalid_value" },
	{ 0xd5f66efd, "cdev_init" },
	{ 0xfbe26b10, "krealloc_noprof" },
	{ 0x4e54d6ac, "cdev_del" },
	{ 0xbebe66ff, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0x9f222e1e,
	0xa61fd7aa,
	0x092a35a2,
	0xd710adbf,
	0xa53f4e29,
	0xcb8b6ec6,
	0xd272d446,
	0xe8213e80,
	0xbd03ed67,
	0xd272d446,
	0x90a48d82,
	0x8ea73856,
	0x366ddfcc,
	0xc1e6c71e,
	0x357aaab3,
	0xe54e0a6b,
	0xd272d446,
	0x092a35a2,
	0x0bc5fb0d,
	0xf46d5bf3,
	0x546c19d9,
	0xe4de56b4,
	0xd5f66efd,
	0xfbe26b10,
	0x4e54d6ac,
	0xbebe66ff,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"alloc_chrdev_region\0"
	"__check_object_size\0"
	"_copy_from_user\0"
	"__kmalloc_noprof\0"
	"memcpy\0"
	"kfree\0"
	"__fentry__\0"
	"_printk\0"
	"__ref_stack_chk_guard\0"
	"__stack_chk_fail\0"
	"__ubsan_handle_out_of_bounds\0"
	"cdev_add\0"
	"memchr\0"
	"__mutex_init\0"
	"mutex_lock_interruptible\0"
	"__fortify_panic\0"
	"__x86_return_thunk\0"
	"_copy_to_user\0"
	"unregister_chrdev_region\0"
	"mutex_unlock\0"
	"validate_usercopy_range\0"
	"__ubsan_handle_load_invalid_value\0"
	"cdev_init\0"
	"krealloc_noprof\0"
	"cdev_del\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "2FBF8E7486CB1D0D0B768FA");
