#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
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
	{ 0x5358b4ed, "module_layout" },
	{ 0x713c860e, "nf_unregister_hook" },
	{ 0xad35242d, "nf_register_hook" },
	{ 0xb8f5f54d, "sock_wfree" },
	{ 0x2124474, "ip_send_check" },
	{ 0x1b6314fd, "in_aton" },
	{ 0x761bbc0b, "skb_push" },
	{ 0xc34e95c4, "skb_realloc_headroom" },
	{ 0xe113bbbc, "csum_partial" },
	{ 0x133346d3, "skb_pull" },
	{ 0x6fd2b9ee, "skb_copy_bits" },
	{ 0x27e1a049, "printk" },
	{ 0xbdfb6dbb, "__fentry__" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "E33973410FD38ACC00A402D");
