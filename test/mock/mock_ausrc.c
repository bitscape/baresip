/**
 * @file mock/mock_ausrc.c Mock audio source
 *
 * Copyright (C) 2010 - 2016 Creytiv.com
 */
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "../test.h"


struct ausrc_st {
	struct tmr tmr;
	struct ausrc_prm prm;
	void *sampv;
	size_t sampc;
	ausrc_read_h *rh;
	void *arg;
};


static void ausrc_destructor(void *arg)
{
	struct ausrc_st *st = arg;

	tmr_cancel(&st->tmr);
	mem_deref(st->sampv);
}


static void tmr_handler(void *arg)
{
	struct ausrc_st *st = arg;
	struct auframe af = {
		.fmt   = st->prm.fmt,
		.sampv = st->sampv,
		.sampc = st->sampc,
		.timestamp = tmr_jiffies_usec()
	};

	tmr_start(&st->tmr, st->prm.ptime, tmr_handler, st);

	if (st->rh)
		st->rh(&af, st->arg);
}


static int mock_ausrc_alloc(struct ausrc_st **stp, const struct ausrc *as,
			    struct media_ctx **ctx,
			    struct ausrc_prm *prm, const char *device,
			    ausrc_read_h *rh, ausrc_error_h *errh, void *arg)
{
	struct ausrc_st *st;
	int err = 0;
	(void)ctx;
	(void)device;
	(void)errh;

	if (!stp || !as || !prm)
		return EINVAL;

	st = mem_zalloc(sizeof(*st), ausrc_destructor);
	if (!st)
		return ENOMEM;

	st->prm  = *prm;
	st->rh   = rh;
	st->arg  = arg;

	st->sampc = prm->srate * prm->ch * prm->ptime / 1000;

	st->sampv = mem_zalloc(aufmt_sample_size(prm->fmt) * st->sampc, NULL);
	if (!st->sampv) {
		err = ENOMEM;
		goto out;
	}

	tmr_start(&st->tmr, 0, tmr_handler, st);

 out:
	if (err)
		mem_deref(st);
	else
		*stp = st;

	return err;
}


int mock_ausrc_register(struct ausrc **ausrcp, struct list *ausrcl)
{
	return ausrc_register(ausrcp, ausrcl,
			      "mock-ausrc", mock_ausrc_alloc);
}
