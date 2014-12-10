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
	{ 0xca05c877, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xe6da44a, __VMLINUX_SYMBOL_STR(set_normalized_timespec) },
	{ 0xcf4306db, __VMLINUX_SYMBOL_STR(netlink_kernel_release) },
	{ 0x920ca1ee, __VMLINUX_SYMBOL_STR(nf_unregister_hook) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xd2b09ce5, __VMLINUX_SYMBOL_STR(__kmalloc) },
	{ 0x1b6314fd, __VMLINUX_SYMBOL_STR(in_aton) },
	{ 0x25563496, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x2f7c36b3, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x35e05ce8, __VMLINUX_SYMBOL_STR(__netlink_kernel_create) },
	{ 0xf0aa16c8, __VMLINUX_SYMBOL_STR(init_net) },
	{ 0x84ea3046, __VMLINUX_SYMBOL_STR(nf_register_hook) },
	{ 0xde7f63da, __VMLINUX_SYMBOL_STR(sock_wfree) },
	{ 0xf0947231, __VMLINUX_SYMBOL_STR(skb_realloc_headroom) },
	{ 0x36511654, __VMLINUX_SYMBOL_STR(skb_copy_bits) },
	{ 0xa2bc235a, __VMLINUX_SYMBOL_STR(__pskb_pull_tail) },
	{ 0x2d8b9432, __VMLINUX_SYMBOL_STR(inet_proto_csum_replace4) },
	{ 0x449ad0a7, __VMLINUX_SYMBOL_STR(memcmp) },
	{ 0x4c8d7cfc, __VMLINUX_SYMBOL_STR(__nlmsg_put) },
	{ 0x5188be86, __VMLINUX_SYMBOL_STR(netlink_unicast) },
	{ 0xf5a8b5b3, __VMLINUX_SYMBOL_STR(__alloc_skb) },
	{ 0x46608fa0, __VMLINUX_SYMBOL_STR(getnstimeofday) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
	{ 0xe113bbbc, __VMLINUX_SYMBOL_STR(csum_partial) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "FD472F512375F4E6783C8BE");
