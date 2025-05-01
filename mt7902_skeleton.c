// mt7902_skeleton.c – Early development driver for MediaTek MT7902 PCIe Wi‑Fi
//
// v0.4 – 01 May 2025
//   * Firmware gerçek DMA indirme & MCU READY bekleme eklendi.
//   * IRQ vektörü devm ile yönetiliyor; kaldırmada çift‐serbest uyarısı giderildi.
//   * Koddan kazara eklenmiş shell satırı çıkarıldı.
//
//  *** TEST HEDEFİ ***
//  dmesg çıktısında sırasıyla aşağıdakileri görmelisiniz:
//      firmware ok: <n> bayt
//      firmware download OK
//      mcu ready OK (state = 0x7)
//
//  Henüz mac80211 entegrasyonu ve RX/TX descriptor’ları yok.
//
// SPDX‑License‑Identifier: GPL‑2.0

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/firmware.h>
#include <linux/etherdevice.h>
#include <linux/delay.h>
#include <linux/iopoll.h>
#include <linux/dma-mapping.h>

#define DRV_NAME        "mt7902_skeleton"
#define MT7902_PCI_VENDOR 0x14c3
#define MT7902_PCI_DEVICE 0x7902
#define MT7902_FW_NAME  "mediatek/mt7902.bin"

/* ───────────── Register offsets (provisional – from MT7921) ───────────── */
#define MT_TOP_MISC2                     0x18060024
#define   MT_TOP_MISC2_FW_STATE          GENMASK(2, 0)   /* 0 = ROM, 7 = RUN */

#define MT_FW_DL_ADDR                    0x1846F200 /* sbus remap */
#define MT_FW_DL_DLEN                    0x1846F204
#define MT_FW_DL_CTRL                    0x1846F208
#define   MT_FW_DL_CTRL_START            BIT(0)
#define   MT_FW_DL_CTRL_ACK              BIT(1)

#define FW_POLL_TIMEOUT_US  100000  /* 100 ms */

MODULE_DESCRIPTION("Skeleton + DMA firmware download + IRQ for MediaTek MT7902");
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

/* ───────────── firmware loader ───────────── */
static int mt7902_load_firmware(struct mt7902_dev *mdev)
{
    const struct firmware *fw;
    dma_addr_t dma_addr;
    void *dma_buf;
    u32 val;
    int ret;

    ret = request_firmware(&fw, MT7902_FW_NAME, &mdev->pdev->dev);
    if (ret) {
        dev_err(&mdev->pdev->dev, "firmware '%s' bulunamadı (ret=%d)\n", MT7902_FW_NAME, ret);
        return ret;
    }

    dev_info(&mdev->pdev->dev, "firmware ok: %zu bayt\n", fw->size);

    /* Coherent DMA buffer ayır ve FW'yi kopyala */
    dma_buf = dma_alloc_coherent(&mdev->pdev->dev, fw->size, &dma_addr, GFP_KERNEL);
    if (!dma_buf) {
        dev_err(&mdev->pdev->dev, "DMA buffer allocate başarısız\n");
        release_firmware(fw);
        return -ENOMEM;
    }
    memcpy(dma_buf, fw->data, fw->size);

    /* Adres & uzunluk yaz, START bitini set et */
    iowrite32(lower_32_bits(dma_addr), mdev->regs + MT_FW_DL_ADDR);
    iowrite32(fw->size,             mdev->regs + MT_FW_DL_DLEN);

    val = ioread32(mdev->regs + MT_FW_DL_CTRL);
    val |= MT_FW_DL_CTRL_START;
    iowrite32(val, mdev->regs + MT_FW_DL_CTRL);

    /* ACK bekle */
    ret = read_poll_timeout(ioread32, val,
                            val & MT_FW_DL_CTRL_ACK,
                            1000, FW_POLL_TIMEOUT_US, false,
                            mdev->regs + MT_FW_DL_CTRL);
    if (ret) {
        dev_err(&mdev->pdev->dev, "FW download ACK timeout\n");
        goto out_free;
    }
    dev_info(&mdev->pdev->dev, "firmware download OK\n");

    /* MCU READY bekle */
    ret = read_poll_timeout(ioread32, val,
                            (val & MT_TOP_MISC2_FW_STATE) == 0x7,
                            1000, FW_POLL_TIMEOUT_US, false,
                            mdev->regs + MT_TOP_MISC2);
    if (ret) {
        dev_err(&mdev->pdev->dev, "MCU READY timeout\n");
        goto out_free;
    }
    dev_info(&mdev->pdev->dev, "mcu ready OK (state=0x%lx)\n", val & MT_TOP_MISC2_FW_STATE);

out_free:
    dma_free_coherent(&mdev->pdev->dev, fw->size, dma_buf, dma_addr);
    release_firmware(fw);
    return ret;
}

/* ───────────── IRQ handler ───────────── */
static irqreturn_t mt7902_irq(int irq, void *ctx)
{
    struct mt7902_dev *mdev = ctx;
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

    /* ── Firmware download ── */
    err = mt7902_load_firmware(mdev);
    if (err)
        goto err_unmap;

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

    err = devm_request_irq(&pdev->dev, mdev->irq_vec, mt7902_irq, 0, DRV_NAME, mdev);
    if (err) {
        dev_err(&pdev->dev, "request_irq başarısız (%d)\n", err);
        goto err_irq;
    }

    pci_set_drvdata(pdev, mdev);
    dev_info(&pdev->dev, "[%s] FW‑DMA + IRQ hazır (vec %d)\n", DRV_NAME, mdev->irq_vec);
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
