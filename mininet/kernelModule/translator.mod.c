#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x2b22e0c3, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xcae3c359, __VMLINUX_SYMBOL_STR(nf_unregister_hook) },
	{ 0xbec3bd0f, __VMLINUX_SYMBOL_STR(nf_register_hook) },
	{ 0xf53d7c5c, __VMLINUX_SYMBOL_STR(sock_wfree) },
	{ 0x2124474, __VMLINUX_SYMBOL_STR(ip_send_check) },
	{ 0x1b6314fd, __VMLINUX_SYMBOL_STR(in_aton) },
	{ 0xc77bace2, __VMLINUX_SYMBOL_STR(skb_push) },
	{ 0x96d93527, __VMLINUX_SYMBOL_STR(skb_realloc_headroom) },
	{ 0xe113bbbc, __VMLINUX_SYMBOL_STR(csum_partial) },
	{ 0x1c3db152, __VMLINUX_SYMBOL_STR(skb_pull) },
	{ 0x1dda7cf0, __VMLINUX_SYMBOL_STR(skb_copy_bits) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "903041D3AF4676D4F7E0F1D");
