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
	{ 0x9412fa01, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xe6da44a, __VMLINUX_SYMBOL_STR(set_normalized_timespec) },
	{ 0x825d6754, __VMLINUX_SYMBOL_STR(netlink_kernel_release) },
	{ 0x4d6b8407, __VMLINUX_SYMBOL_STR(nf_unregister_hook) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xd2b09ce5, __VMLINUX_SYMBOL_STR(__kmalloc) },
	{ 0x1b6314fd, __VMLINUX_SYMBOL_STR(in_aton) },
	{ 0x6e938e79, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x6d0fc37d, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0xd1946d9b, __VMLINUX_SYMBOL_STR(__netlink_kernel_create) },
	{ 0xec76d1f9, __VMLINUX_SYMBOL_STR(init_net) },
	{ 0x72a81750, __VMLINUX_SYMBOL_STR(nf_register_hook) },
	{ 0x2aa5c788, __VMLINUX_SYMBOL_STR(sock_wfree) },
	{ 0x7ec9e0c1, __VMLINUX_SYMBOL_STR(skb_realloc_headroom) },
	{ 0x9c66a3ae, __VMLINUX_SYMBOL_STR(skb_copy_bits) },
	{ 0xcb4c886c, __VMLINUX_SYMBOL_STR(__pskb_pull_tail) },
	{ 0x3c8ed625, __VMLINUX_SYMBOL_STR(inet_proto_csum_replace4) },
	{ 0x449ad0a7, __VMLINUX_SYMBOL_STR(memcmp) },
	{ 0xa41cf6ec, __VMLINUX_SYMBOL_STR(__nlmsg_put) },
	{ 0x6f249ee9, __VMLINUX_SYMBOL_STR(netlink_unicast) },
	{ 0x1084e713, __VMLINUX_SYMBOL_STR(__alloc_skb) },
	{ 0x46608fa0, __VMLINUX_SYMBOL_STR(getnstimeofday) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
	{ 0xe113bbbc, __VMLINUX_SYMBOL_STR(csum_partial) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "0C047A7FBF14FD719825021");
