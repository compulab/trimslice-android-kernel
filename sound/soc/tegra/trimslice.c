/*
 * trimslice.c - TrimSlice machine ASoC driver
 *
 * Copyright (C) 2011 - CompuLab, Ltd.
 * Author: Valentin Raevsky <valentin@compulab.co.il>
 *
 * Based on code copyright/by:
 * Author: Stephen Warren <swarren@nvidia.com>
 * Copyright (C) 2010-2011 - NVIDIA, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <asm/mach-types.h>

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <sound/core.h>
#include <sound/jack.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "../codecs/tlv320aic23.h"

#include "tegra_das.h"
#include "tegra_i2s.h"
#include "tegra_pcm.h"
#include "tegra_asoc_utils.h"

#define DRV_NAME "tegra-snd-trimslice"

#define SND_ANALOG  0
#define SND_DIGITAL 1

struct tegra_trimslice {
	struct tegra_asoc_utils_data util_data;
};

static int trimslice_asoc_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_card *card = codec->card;
	struct tegra_trimslice *trimslice = snd_soc_card_get_drvdata(card);
	int srate, mclk;
	int err;

	srate = params_rate(params);
	switch (srate) {
	case 64000:
	case 88200:
	case 96000:
		mclk = 128 * srate;
		break;
	default:
		mclk = 256 * srate;
		break;
	}
	/* FIXME: Codec only requires >= 3MHz if OSR==0 */
	while (mclk < 6000000)
		mclk *= 2;

	err = tegra_asoc_utils_set_rate(&trimslice->util_data, srate, mclk);

	if (err < 0) {
		dev_err(card->dev, "Can't configure clocks\n");
		return err;
	}

	err = snd_soc_dai_set_fmt(codec_dai,
					SND_SOC_DAIFMT_I2S |
					SND_SOC_DAIFMT_NB_NF |
					SND_SOC_DAIFMT_CBS_CFS);
	if (err < 0) {
		dev_err(card->dev, "codec_dai fmt not set\n");
		return err;
	}

	err = snd_soc_dai_set_fmt(cpu_dai,
					SND_SOC_DAIFMT_I2S |
					SND_SOC_DAIFMT_NB_NF |
					SND_SOC_DAIFMT_CBS_CFS);
	if (err < 0) {
		dev_err(card->dev, "cpu_dai fmt not set\n");
		return err;
	}

	/* configure DAI master clock */
	err = snd_soc_dai_set_sysclk(codec_dai, 0, mclk,
					SND_SOC_CLOCK_IN);
	if (err < 0) {
		dev_err(card->dev, "codec_dai clock not set\n");
		return err;
	}

	return 0;
}

static int trimslice_spdif_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_card *card = codec->card;
	struct tegra_trimslice *trimslice = snd_soc_card_get_drvdata(card);
	int srate, mclk;
	int err;

	/*
	 * FIXME: Refactor mclk into PCM-specific function; SPDIF doesn't
	 * need it
	 */
	srate = params_rate(params);
	switch (srate) {
	case 64000:
	case 88200:
	case 96000:
		mclk = 128 * srate;
		break;
	default:
		mclk = 256 * srate;
		break;
	}
	/* FIXME: Codec only requires >= 3MHz if OSR==0 */
	while (mclk < 6000000)
		mclk *= 2;

	err = tegra_asoc_utils_set_rate(&trimslice->util_data, srate, mclk);
	if (err < 0) {
		dev_err(card->dev, "Can't configure clocks\n");
		return err;
	}
	return 0;
}

static struct snd_soc_ops trimslice_asoc_ops = {
	.hw_params = trimslice_asoc_hw_params,
};

static const struct snd_soc_dapm_widget trimslice_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Line Out", NULL),
	SND_SOC_DAPM_LINE("Line In", NULL),
};

static const struct snd_soc_dapm_route trimslice_audio_map[] = {
	{"Line Out", NULL, "LOUT"},
	{"Line Out", NULL, "ROUT"},

	{"LLINEIN", NULL, "Line In"},
	{"RLINEIN", NULL, "Line In"},
};

static int trimslice_asoc_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	snd_soc_dapm_nc_pin(dapm, "LHPOUT");
	snd_soc_dapm_nc_pin(dapm, "RHPOUT");
	snd_soc_dapm_nc_pin(dapm, "MICIN");

	snd_soc_dapm_sync(dapm);

	return 0;
}

static struct snd_soc_ops trimslice_spdif_ops = {
	.hw_params = trimslice_spdif_hw_params,
};

static struct snd_soc_dai_link trimslice_tlv320aic23_dai[] = {
{    /* analog */
	.name = "TLV320AIC23",
	.stream_name = "AIC23",
	.codec_name = "tlv320aic23-codec.2-001a",
	.platform_name = "tegra-pcm-audio",
	.cpu_dai_name = "tegra-i2s.0",
	.codec_dai_name = "tlv320aic23-hifi",
	.init = trimslice_asoc_init,
	.ops = &trimslice_asoc_ops,
},
{    /* digital */
	.name = "SPDIF",
	.stream_name = "spdif",
	.codec_name = "spdif-dit",
	.platform_name = "tegra-pcm-audio",
	.cpu_dai_name = "tegra-spdif",
	.codec_dai_name = "dit-hifi",
	.ops = &trimslice_spdif_ops,
}
};


static struct snd_soc_card snd_soc_trimslice_cards[] = {
{
	.name = "tegra-trimslice-analog",
	.dai_link = &trimslice_tlv320aic23_dai[SND_ANALOG],
	.num_links = 1,

	.dapm_widgets = trimslice_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(trimslice_dapm_widgets),
	.dapm_routes = trimslice_audio_map,
	.num_dapm_routes = ARRAY_SIZE(trimslice_audio_map),
},
{
	.name = "tegra-trimslice-digital",
	.dai_link = &trimslice_tlv320aic23_dai[SND_DIGITAL],
	.num_links = 1,
}
};

static __devinit int tegra_snd_trimslice_probe0(struct platform_device *pdev)
{
	struct snd_soc_card *card = &snd_soc_trimslice_cards[SND_ANALOG];
	struct tegra_trimslice *trimslice = NULL;
	int ret;

	trimslice = kzalloc(sizeof(struct tegra_trimslice), GFP_KERNEL);
	if (!trimslice) {
		dev_err(&pdev->dev, "Can't allocate tegra_trimslice\n");
		return -ENOMEM;
	}

	ret = tegra_asoc_utils_init(&trimslice->util_data, &pdev->dev);
	if (ret)
		goto err_free_trimslice;

	platform_set_drvdata(pdev, card);

	card = &snd_soc_trimslice_cards[SND_ANALOG];
	card->dev = &pdev->dev;
	snd_soc_card_set_drvdata(card, trimslice);
	ret = snd_soc_register_card(card);
	if (ret) {
		dev_err(&pdev->dev, "snd_soc_register_card failed (%d)\n",
			ret);
		goto err_fini_utils;
	}

	return 0;

err_fini_utils:
	tegra_asoc_utils_fini(&trimslice->util_data);
err_free_trimslice:
	kfree(trimslice);
	return ret;
}

static __devinit int tegra_snd_trimslice_probe1(struct platform_device *pdev)
{
	struct snd_soc_card *card = &snd_soc_trimslice_cards[SND_DIGITAL];
	struct tegra_trimslice *trimslice = NULL;
	int ret;

	trimslice = kzalloc(sizeof(struct tegra_trimslice), GFP_KERNEL);
	if (!trimslice) {
		dev_err(&pdev->dev, "Can't allocate tegra_trimslice\n");
		return -ENOMEM;
	}

	ret = tegra_asoc_utils_init(&trimslice->util_data, &pdev->dev);
	if (ret)
		goto err_free_trimslice;

	platform_set_drvdata(pdev, card);

	card = &snd_soc_trimslice_cards[SND_DIGITAL];
	card->dev = &pdev->dev;
	snd_soc_card_set_drvdata(card, trimslice);
	ret = snd_soc_register_card(card);
	if (ret) {
		dev_err(&pdev->dev, "snd_soc_register_card failed (%d)\n",
			ret);
		goto err_fini_utils;
	}

	return 0;

err_fini_utils:
	tegra_asoc_utils_fini(&trimslice->util_data);
err_free_trimslice:
	kfree(trimslice);
	return ret;
}

static int __devexit tegra_snd_trimslice_remove0(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	struct tegra_trimslice *trimslice = snd_soc_card_get_drvdata(card);

	snd_soc_unregister_card(&card[SND_ANALOG]);

	tegra_asoc_utils_fini(&trimslice->util_data);

	kfree(trimslice);

	return 0;
}

static int __devexit tegra_snd_trimslice_remove1(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	struct tegra_trimslice *trimslice = snd_soc_card_get_drvdata(card);

	snd_soc_unregister_card(&card[SND_DIGITAL]);

	tegra_asoc_utils_fini(&trimslice->util_data);

	kfree(trimslice);

	return 0;
}

static int __devexit tegra_snd_trimslice_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	struct tegra_trimslice *trimslice = snd_soc_card_get_drvdata(card);

	snd_soc_unregister_card(&card[SND_ANALOG]);
	snd_soc_unregister_card(&card[SND_DIGITAL]);

	tegra_asoc_utils_fini(&trimslice->util_data);

	kfree(trimslice);

	return 0;
}

static struct platform_driver tegra_snd_trimslice_driver0 = {
	.driver = {
		.name = DRV_NAME"-0",
		.owner = THIS_MODULE,
	},
	.probe = tegra_snd_trimslice_probe0,
	.remove = __devexit_p(tegra_snd_trimslice_remove0),
};

static struct platform_driver tegra_snd_trimslice_driver1 = {
	.driver = {
		.name = DRV_NAME"-1",
		.owner = THIS_MODULE,
	},
	.probe = tegra_snd_trimslice_probe1,
	.remove = __devexit_p(tegra_snd_trimslice_remove1),
};

static int __init snd_tegra_trimslice_init(void)
{
		int ret = 0;
		ret = platform_driver_register(&tegra_snd_trimslice_driver0);
		if (ret)
			return ret;

		ret = platform_driver_register(&tegra_snd_trimslice_driver1);
		if (ret)
			platform_driver_unregister(&tegra_snd_trimslice_driver0);

		return ret;
}
module_init(snd_tegra_trimslice_init);

static void __exit snd_tegra_trimslice_exit(void)
{
	platform_driver_unregister(&tegra_snd_trimslice_driver0);
	platform_driver_unregister(&tegra_snd_trimslice_driver1);
}
module_exit(snd_tegra_trimslice_exit);

MODULE_AUTHOR("Valentin Raevsky <valentin@compulab.co.il>");
MODULE_DESCRIPTION("Trimslice machine ASoC driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME"-0");
MODULE_ALIAS("platform:" DRV_NAME"-1");
