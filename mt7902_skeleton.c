// mt7902_skeleton.c – Minimal PCIe skeleton driver for MediaTek MT7902 Wi‑Fi card
//
// v0.2.1 – 01 May 2025
//   * IRQ tahsis bayrağında PCI_IRQ_LEGACY derleyicide mevcut değilse
//     PCI_IRQ_ALL_TYPES kullanılıyor (çekirdek 6.11+’de isim değişikliği).
//   * Hata yönetimi mesajları minör iyileştirildi.
//
// SPDX‑License‑Identifier: GPL‑2.0

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/etherdevice.h>

#define DRV_NAME "mt7902_skeleton"
#define MT7902_PCI_VENDOR 0x14c3
#define MT7902_PCI_DEVICE 0x7902

MODULE_DESCRIPTION("Skeleton + IRQ handler for MediaTek MT7902");
MODULE_AUTHOR("<Your Name>");
MODULE_LICENSE("GPL");

/* ───────────── PCI ID table ───────────── */
static const struct pci_device_id mt7902_pci_table[] = {
    { PCI_DEVICE(MT7902_PCI_VENDOR, MT7902_PCI_DEVICE) },
    { }
};
MODULE_DEVICE_TABLE(pci, mt7902_pci_table);

/* ───────────── private state ───────────── */
struct mt7902_dev {
    struct pci_dev *pdev;
    void __iomem   *regs;
    int             irq_vec;
};

/* ───────────── IRQ handler ───────────── */
static irqreturn_t mt7902_irq(int irq, void *ctx)
{
    struct mt7902_dev *mdev = ctx;

    /* TODO: IRQ status register clear */
    dev_dbg(&mdev->pdev->dev, "IRQ hit (vec=%d)\n", irq);
    return IRQ_HANDLED;
}

/* ───────────── probe / remove ───────────── */
static int mt7902_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    struct mt7902_dev *mdev;
    int err;

    dev_info(&pdev->dev, "[%s] probe\n", DRV_NAME);

    err = pci_enable_device(pdev);
    if (err)
        return err;

    err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
    if (err) {
        dev_err(&pdev->dev, "DMA mask 32‑bit yapılamadı (%d)\n", err);
        goto err_disable;
    }

    err = pci_request_regions(pdev, DRV_NAME);
    if (err)
        goto err_disable;

    mdev = devm_kzalloc(&pdev->dev, sizeof(*mdev), GFP_KERNEL);
    if (!mdev) {
        err = -ENOMEM;
        goto err_release;
    }
    mdev->pdev = pdev;

    mdev->regs = pci_ioremap_bar(pdev, 0);
    if (!mdev->regs) {
        err = -EIO;
        goto err_release;
    }

    pci_set_master(pdev);

    /* ── IRQ vektör tahsisi ── */
#ifdef PCI_IRQ_ALL_TYPES
    err = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_ALL_TYPES);
#else
    err = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_MSIX | PCI_IRQ_MSI);
#endif
    if (err < 0) {
        dev_err(&pdev->dev, "uygun IRQ vektörü yok (%d)\n", err);
        goto err_unmap;
    }
    mdev->irq_vec = pci_irq_vector(pdev, 0);

    err = devm_request_irq(&pdev->dev, mdev->irq_vec, mt7902_irq,
                           0, DRV_NAME, mdev);
    if (err) {
        dev_err(&pdev->dev, "request_irq başarısız (%d)\n", err);
        goto err_irq;
    }

    pci_set_drvdata(pdev, mdev);
    dev_info(&pdev->dev, "[%s] skeleton + IRQ hazır (vec %d)\n", DRV_NAME, mdev->irq_vec);
    return 0;

err_irq:
    pci_free_irq_vectors(pdev);
err_unmap:
    iounmap(mdev->regs);
err_release:
    pci_release_regions(pdev);
err_disable:
    pci_disable_device(pdev);
    return err;
}

static void mt7902_remove(struct pci_dev *pdev)
{
    struct mt7902_dev *mdev = pci_get_drvdata(pdev);

    dev_info(&pdev->dev, "[%s] remove\n", DRV_NAME);

    if (mdev->regs)
        iounmap(mdev->regs);

    pci_free_irq_vectors(pdev);
    pci_release_regions(pdev);
    pci_disable_device(pdev);
}

/* ───────────── driver struct ───────────── */
static struct pci_driver mt7902_pci_driver = {
    .name     = DRV_NAME,
    .id_table = mt7902_pci_table,
    .probe    = mt7902_probe,
    .remove   = mt7902_remove,
};

module_pci_driver(mt7902_pci_driver);
