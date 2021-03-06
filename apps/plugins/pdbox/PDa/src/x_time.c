/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* clock objects */

#include "m_pd.h"
#include <stdio.h>
/* -------------------------- delay ------------------------------ */
static t_class *delay_class;

typedef struct _delay
{
    t_object x_obj;
    t_clock *x_clock;
    double x_deltime;
} t_delay;

static void delay_bang(t_delay *x)
{
    clock_delay(x->x_clock, x->x_deltime);
}

static void delay_stop(t_delay *x)
{
    clock_unset(x->x_clock);
}

static void delay_ft1(t_delay *x, t_floatarg g)
{
    if (g < 0) g = 0;
    x->x_deltime = g;
}

static void delay_float(t_delay *x, t_float f)
{
    delay_ft1(x, f);
    delay_bang(x);
}

static void delay_tick(t_delay *x)
{
    outlet_bang(x->x_obj.ob_outlet);
}

static void delay_free(t_delay *x)
{
    clock_free(x->x_clock);
}

static void *delay_new(t_floatarg f)
{
    t_delay *x = (t_delay *)pd_new(delay_class);
    delay_ft1(x, f);
    x->x_clock = clock_new(x, (t_method)delay_tick);
    outlet_new(&x->x_obj, gensym("bang"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("ft1"));
    return (x);
}

static void delay_setup(void)
{
    delay_class = class_new(gensym("delay"), (t_newmethod)delay_new,
    	(t_method)delay_free, sizeof(t_delay), 0, A_DEFFLOAT, 0);
    class_addcreator((t_newmethod)delay_new, gensym("del"), A_DEFFLOAT, 0);
    class_addbang(delay_class, delay_bang);
    class_addmethod(delay_class, (t_method)delay_stop, gensym("stop"), 0);
    class_addmethod(delay_class, (t_method)delay_ft1,
    	gensym("ft1"), A_FLOAT, 0);
    class_addfloat(delay_class, (t_method)delay_float);
}

/* -------------------------- metro ------------------------------ */
static t_class *metro_class;

typedef struct _metro
{
    t_object x_obj;
    t_clock *x_clock;
    double x_deltime;
    int x_hit;
} t_metro;

static void metro_tick(t_metro *x)
{
    x->x_hit = 0;
    outlet_bang(x->x_obj.ob_outlet);
    if (!x->x_hit) clock_delay(x->x_clock, x->x_deltime);
}

static void metro_float(t_metro *x, t_float f)
{
    if (f != 0) metro_tick(x);
    else clock_unset(x->x_clock);
    x->x_hit = 1;
}

static void metro_bang(t_metro *x)
{
    metro_float(x, 1);
}

static void metro_stop(t_metro *x)
{
    metro_float(x, 0);
}

static void metro_ft1(t_metro *x, t_floatarg g)
{
    if (g < 1) g = 1;
    x->x_deltime = g;
}

static void metro_free(t_metro *x)
{
    clock_free(x->x_clock);
}

static void *metro_new(t_floatarg f)
{
    t_metro *x = (t_metro *)pd_new(metro_class);
    metro_ft1(x, f);
    x->x_hit = 0;
    x->x_clock = clock_new(x, (t_method)metro_tick);
    outlet_new(&x->x_obj, gensym("bang"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("ft1"));
    return (x);
}

static void metro_setup(void)
{
    metro_class = class_new(gensym("metro"), (t_newmethod)metro_new,
    	(t_method)metro_free, sizeof(t_metro), 0, A_DEFFLOAT, 0);
    class_addbang(metro_class, metro_bang);
    class_addmethod(metro_class, (t_method)metro_stop, gensym("stop"), 0);
    class_addmethod(metro_class, (t_method)metro_ft1, gensym("ft1"),
    	A_FLOAT, 0);
    class_addfloat(metro_class, (t_method)metro_float);
}

/* -------------------------- line ------------------------------ */
static t_class *line_class;

typedef struct _line
{
    t_object x_obj;
    t_clock *x_clock;
    double x_targettime;
    t_float x_targetval;
    double x_prevtime;
    t_float x_setval;
    int x_gotinlet;
    t_float x_grain;
    double x_1overtimediff;
    double x_in1val;
} t_line;

static void line_tick(t_line *x)
{
    double timenow = clock_getsystime();
    double msectogo = - clock_gettimesince(x->x_targettime);
    if (msectogo < 1E-9)
    {
    	outlet_float(x->x_obj.ob_outlet, x->x_targetval);
    }
    else
    {
    	outlet_float(x->x_obj.ob_outlet,
    	    x->x_setval + x->x_1overtimediff * (timenow - x->x_prevtime)
    	    	* (x->x_targetval - x->x_setval));
    	clock_delay(x->x_clock,
    	    (x->x_grain > msectogo ? msectogo : x->x_grain));
    }
}

static void line_float(t_line *x, t_float f)
{
    double timenow = clock_getsystime();
    if (x->x_gotinlet && x->x_in1val > 0)
    {
    	if (timenow > x->x_targettime) x->x_setval = x->x_targetval;
    	else x->x_setval = x->x_setval + x->x_1overtimediff *
    	    (timenow - x->x_prevtime)
    	    * (x->x_targetval - x->x_setval);
    	x->x_prevtime = timenow;
    	x->x_targettime = clock_getsystimeafter(x->x_in1val);
    	x->x_targetval = f;
    	line_tick(x);
    	x->x_gotinlet = 0;
    	x->x_1overtimediff = 1./ (x->x_targettime - timenow);
    	clock_delay(x->x_clock,
    	    (x->x_grain > x->x_in1val ? x->x_in1val : x->x_grain));
    
    }
    else
    {
    	clock_unset(x->x_clock);
    	x->x_targetval = x->x_setval = f;
    	outlet_float(x->x_obj.ob_outlet, f);
    }
    x->x_gotinlet = 0;
}

static void line_ft1(t_line *x, t_floatarg g)
{
    x->x_in1val = g;
    x->x_gotinlet = 1;
}

static void line_stop(t_line *x)
{
    x->x_targetval = x->x_setval;
    clock_unset(x->x_clock);
}

static void line_free(t_line *x)
{
    clock_free(x->x_clock);
}

static void *line_new(t_floatarg f, t_floatarg grain)
{
    t_line *x = (t_line *)pd_new(line_class);
    x->x_targetval = x->x_setval = f;
    x->x_gotinlet = 0;
    x->x_1overtimediff = 1;
    x->x_clock = clock_new(x, (t_method)line_tick);
    x->x_targettime = x->x_prevtime = clock_getsystime();
    if (grain <= 0) grain = 20;
    x->x_grain = grain;
    outlet_new(&x->x_obj, gensym("float"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("ft1"));
    return (x);
}

static void line_setup(void)
{
    line_class = class_new(gensym("line"), (t_newmethod)line_new,
    	(t_method)line_free, sizeof(t_line), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(line_class, (t_method)line_ft1,
    	gensym("ft1"), A_FLOAT, 0);
    class_addmethod(line_class, (t_method)line_stop,
    	gensym("stop"), 0);
    class_addfloat(line_class, (t_method)line_float);
}

/* -------------------------- timer ------------------------------ */
static t_class *timer_class;

typedef struct _timer
{
    t_object x_obj;
    t_time x_settime;
} t_timer;

static void timer_bang(t_timer *x)
{
    x->x_settime = clock_getsystime();
}

static void timer_bang2(t_timer *x)
{
    t_time diff = clock_gettimesince(x->x_settime);
    outlet_float(x->x_obj.ob_outlet, diff);
}

static void *timer_new(t_floatarg f)
{
#ifdef ROCKBOX
    (void) f;
#endif
    t_timer *x = (t_timer *)pd_new(timer_class);
    timer_bang(x);
    outlet_new(&x->x_obj, gensym("float"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("bang"), gensym("bang2"));
    return (x);
}

static void timer_setup(void)
{
    timer_class = class_new(gensym("timer"), (t_newmethod)timer_new, 0,
    	sizeof(t_timer), 0, A_DEFFLOAT, 0);
    class_addbang(timer_class, timer_bang);
    class_addmethod(timer_class, (t_method)timer_bang2, gensym("bang2"), 0);
}


/* -------------------------- pipe -------------------------- */

static t_class *pipe_class;

typedef struct _hang
{
    t_clock *h_clock;
    struct _hang *h_next;
    struct _pipe *h_owner;
    t_gpointer *h_gp;
    union word h_vec[1];    	/* not the actual number. */
} t_hang;

typedef struct pipeout
{
    t_atom p_atom;
    t_outlet *p_outlet;
} t_pipeout;

typedef struct _pipe
{
    t_object x_obj;
    int x_n;
    int x_nptr;
    float x_deltime;
    t_pipeout *x_vec;
    t_gpointer *x_gp;
    t_hang *x_hang;
} t_pipe;

static void *pipe_new(t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
#endif
    t_pipe *x = (t_pipe *)pd_new(pipe_class);
    t_atom defarg, *ap;
    t_pipeout *vec, *vp;
    t_gpointer *gp;
    int nptr = 0;
    int i;
    float deltime;
    if (argc)
    {
    	if (argv[argc-1].a_type != A_FLOAT)
	{
    	    char stupid[80];
    	    atom_string(&argv[argc-1], stupid, 79);
    	    post("pipe: %s: bad time delay value", stupid);
    	    deltime = 0;
	}
	else deltime = argv[argc-1].a_w.w_float;
	argc--;
    }
    else deltime = 0;
    if (!argc)
    {
    	argv = &defarg;
    	argc = 1;
    	SETFLOAT(&defarg, 0);
    }
    x->x_n = argc;
    vec = x->x_vec = (t_pipeout *)getbytes(argc * sizeof(*x->x_vec));

    for (i = argc, ap = argv; i--; ap++)
    	if (ap->a_type == A_SYMBOL && *ap->a_w.w_symbol->s_name == 'p')
    	    nptr++;

    gp = x->x_gp = (t_gpointer *)t_getbytes(nptr * sizeof (*gp));
    x->x_nptr = nptr;

    for (i = 0, vp = vec, ap = argv; i < argc; i++, ap++, vp++)
    {
    	if (ap->a_type == A_FLOAT)
    	{
    	    vp->p_atom = *ap;
    	    vp->p_outlet = outlet_new(&x->x_obj, &s_float);
    	    if (i) floatinlet_new(&x->x_obj, &vp->p_atom.a_w.w_float);
    	}
    	else if (ap->a_type == A_SYMBOL)
    	{
    	    char c = *ap->a_w.w_symbol->s_name;
    	    if (c == 's')
    	    {
    	    	SETSYMBOL(&vp->p_atom, &s_symbol);
    	    	vp->p_outlet = outlet_new(&x->x_obj, &s_symbol);
    	    	if (i) symbolinlet_new(&x->x_obj, &vp->p_atom.a_w.w_symbol);
    	    }
    	    else if (c == 'p')
    	    {
    	    	vp->p_atom.a_type = A_POINTER;
    	    	vp->p_atom.a_w.w_gpointer = gp;
    	    	gpointer_init(gp);
    	    	vp->p_outlet = outlet_new(&x->x_obj, &s_pointer);
    	    	if (i) pointerinlet_new(&x->x_obj, gp);
    	    	gp++;
    	    }
    	    else
    	    {
    	    	if (c != 'f') error("pack: %s: bad type",
    	    	    ap->a_w.w_symbol->s_name);
    	    	SETFLOAT(&vp->p_atom, 0);
    	    	vp->p_outlet = outlet_new(&x->x_obj, &s_float);
    	    	if (i) floatinlet_new(&x->x_obj, &vp->p_atom.a_w.w_float);
    	    }
    	}
    }
    floatinlet_new(&x->x_obj, &x->x_deltime);
    x->x_hang = 0;
    x->x_deltime = deltime;
    return (x);
}

static void hang_free(t_hang *h)
{
    t_pipe *x = h->h_owner;
    t_gpointer *gp;
    int i;
    for (gp = h->h_gp, i = x->x_nptr; i--; gp++)
    	gpointer_unset(gp);
    freebytes(h->h_gp, x->x_nptr * sizeof(*h->h_gp));
    clock_free(h->h_clock);
    freebytes(h, sizeof(*h) + (x->x_n - 1) * sizeof(*h->h_vec));
}

static void hang_tick(t_hang *h)
{
    t_pipe *x = h->h_owner;
    t_hang *h2, *h3;
    t_pipeout *p;
    int i;
    union word *w;
    if (x->x_hang == h) x->x_hang = h->h_next;
    else for (h2 = x->x_hang; (h3 = h2->h_next); h2 = h3)
    {
    	if (h3 == h)
    	{
    	    h2->h_next = h3->h_next;
    	    break;
    	}
    }
    for (i = x->x_n, p = x->x_vec + (x->x_n - 1), w = h->h_vec + (x->x_n - 1);
    	i--; p--, w--)
    {
    	switch (p->p_atom.a_type)
    	{
    	case A_FLOAT: outlet_float(p->p_outlet, w->w_float); break;
    	case A_SYMBOL: outlet_symbol(p->p_outlet, w->w_symbol); break;
    	case A_POINTER:
    	    if (gpointer_check(w->w_gpointer, 1))
    	    	outlet_pointer(p->p_outlet, w->w_gpointer);
    	    else post("pipe: stale pointer");
    	    break;
#ifdef ROCKBOX
        default: break;
#endif
    	}
    }
    hang_free(h);
}

static void pipe_list(t_pipe *x, t_symbol *s, int ac, t_atom *av)
{
#ifdef ROCKBOX
    (void) s;
#endif
    t_hang *h = (t_hang *)
    	getbytes(sizeof(*h) + (x->x_n - 1) * sizeof(*h->h_vec));
    t_gpointer *gp, *gp2;
    t_pipeout *p;
    int i, n = x->x_n;
    t_atom *ap;
    t_word *w;
    h->h_gp = (t_gpointer *)getbytes(x->x_nptr * sizeof(t_gpointer));
    if (ac > n) ac = n;
    for (i = 0, gp = x->x_gp, p = x->x_vec, ap = av; i < ac;
    	i++, p++, ap++)
    {
    	switch (p->p_atom.a_type)
    	{
    	case A_FLOAT: p->p_atom.a_w.w_float = atom_getfloat(ap); break;
    	case A_SYMBOL: p->p_atom.a_w.w_symbol = atom_getsymbol(ap); break;
    	case A_POINTER:
    	    gpointer_unset(gp);
    	    if (ap->a_type != A_POINTER)
    	    	post("pipe: bad pointer");
    	    else
    	    {
    	    	*gp = *(ap->a_w.w_gpointer);
    	    	if (gp->gp_stub) gp->gp_stub->gs_refcount++;
    	    }
    	    gp++;
#ifdef ROCKBOX
            break;
        default: break;
#endif
    	}
    }
    for (i = 0, gp = x->x_gp, gp2 = h->h_gp, p = x->x_vec, w = h->h_vec;
    	i < n; i++, p++, w++)
    {
    	if (p->p_atom.a_type == A_POINTER)
    	{
    	    if (gp->gp_stub) gp->gp_stub->gs_refcount++;
    	    w->w_gpointer = gp2;
    	    *gp2++ = *gp++;
    	}
    	else *w = p->p_atom.a_w;
    }
    h->h_next = x->x_hang;
    x->x_hang = h;
    h->h_owner = x;
    h->h_clock = clock_new(h, (t_method)hang_tick);
    clock_delay(h->h_clock, (x->x_deltime >= 0 ? x->x_deltime : 0));
}

static void pipe_flush(t_pipe *x)
{
    while (x->x_hang) hang_tick(x->x_hang);
}

static void pipe_clear(t_pipe *x)
{
    t_hang *hang;
    while ((hang = x->x_hang))
    {
    	x->x_hang = hang->h_next;
    	hang_free(hang);
    }
}

static void pipe_setup(void)
{
    pipe_class = class_new(gensym("pipe"), 
    	(t_newmethod)pipe_new, (t_method)pipe_clear,
    	sizeof(t_pipe), 0, A_GIMME, 0);
    class_addlist(pipe_class, pipe_list);
    class_addmethod(pipe_class, (t_method)pipe_flush, gensym("flush"), 0);
    class_addmethod(pipe_class, (t_method)pipe_clear, gensym("clear"), 0);
}

void x_time_setup(void)
{
    delay_setup();
    metro_setup();
    line_setup();
    timer_setup();
    pipe_setup();
}

