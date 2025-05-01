#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
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

#ifdef CONFIG_MITIGATION_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif



static const char ____versions[]
__used __section("__versions") =
	"\x14\x00\x00\x00\xb0\x75\x9e\x71"
	"_dev_info\0\0\0"
	"\x10\x00\x00\x00\x53\x39\xc0\xed"
	"iounmap\0"
	"\x20\x00\x00\x00\x2d\x4f\x24\x8e"
	"pci_free_irq_vectors\0\0\0\0"
	"\x1c\x00\x00\x00\xcf\x78\x6e\x10"
	"pci_release_regions\0"
	"\x1c\x00\x00\x00\x0c\xa9\x59\x26"
	"pci_disable_device\0\0"
	"\x20\x00\x00\x00\x49\x1a\x8f\x46"
	"pci_unregister_driver\0\0\0"
	"\x1c\x00\x00\x00\x30\x09\xa1\x0f"
	"__dynamic_dev_dbg\0\0\0"
	"\x1c\x00\x00\x00\xb8\xc7\x17\xfb"
	"pci_enable_device\0\0\0"
	"\x18\x00\x00\x00\xf5\x68\x1f\x9a"
	"dma_set_mask\0\0\0\0"
	"\x14\x00\x00\x00\x38\x24\xc3\xb0"
	"_dev_err\0\0\0\0"
	"\x20\x00\x00\x00\x30\xf2\x2d\xdb"
	"dma_set_coherent_mask\0\0\0"
	"\x1c\x00\x00\x00\x31\xe0\x27\xa7"
	"pci_request_regions\0"
	"\x18\x00\x00\x00\x52\x5a\x04\x56"
	"devm_kmalloc\0\0\0\0"
	"\x18\x00\x00\x00\x44\xdf\x12\x4f"
	"pci_ioremap_bar\0"
	"\x18\x00\x00\x00\x9b\xf3\xb1\x61"
	"pci_set_master\0\0"
	"\x20\x00\x00\x00\xfc\xac\x3c\xeb"
	"pci_alloc_irq_vectors\0\0\0"
	"\x18\x00\x00\x00\xbc\x39\x38\xcf"
	"pci_irq_vector\0\0"
	"\x24\x00\x00\x00\xae\x9d\xdc\x45"
	"devm_request_threaded_irq\0\0\0"
	"\x14\x00\x00\x00\xbb\x6d\xfb\xbd"
	"__fentry__\0\0"
	"\x20\x00\x00\x00\x1f\x5c\x90\xf0"
	"__pci_register_driver\0\0\0"
	"\x1c\x00\x00\x00\xca\x39\x82\x5b"
	"__x86_return_thunk\0\0"
	"\x18\x00\x00\x00\xde\x9f\x8a\x25"
	"module_layout\0\0\0"
	"\x00\x00\x00\x00\x00\x00\x00\x00";

MODULE_INFO(depends, "");

MODULE_ALIAS("pci:v000014C3d00007902sv*sd*bc*sc*i*");

MODULE_INFO(srcversion, "821B55A3B5635391B31F4B5");
