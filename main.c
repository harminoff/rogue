/*
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 *
 * @(#)main.c	4.22 (Berkeley) 02/05/99
 */

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <curses.h>
#include "rogue.h"
#include "frontend.h"
#include "variant.h"

int rogue52_main(int argc, char **argv, char **envp);
int rogue36_main(int argc, char **argv, char **envp);
int srogue90_main(int argc, char **argv, char **envp);

static void
prepare_rogue36_shared_state(void)
{
    /*
     * MinGW emits .refptr globals for some 3.6.2 data references that are
     * not renamed by the variant objcopy pass. Seed the live root hunger
     * state so the 3.6.2 stomach daemon does not start at zero food.
     */
    food_left = HUNGERTIME;
    hungry_state = 0;
    no_food = 0;
}

/*
 * main:
 *	The main program, of course
 */
int
main(int argc, char **argv, char **envp)
{
    char *env;
    int lowtime;

    md_init();
    if (!rogue_variant_init_args(&argc, argv))
    {
	fprintf(stderr, "%s\n", rogue_variant_error());
	my_exit(1);
    }
    if (!rogue_frontend_init(&argc, argv))
	my_exit(1);
    if (rogue_frontend_is_tiles() && !rogue_variant_was_explicit())
    {
	if (!rogue_frontend_start())
	    my_exit(1);
	if (!rogue_frontend_choose_variant())
	    my_exit(1);
    }

    if (rogue_variant_is_current("rogue52"))
    {
	return rogue52_main(argc, argv, envp);
    }
    if (rogue_variant_is_current("rogue36"))
    {
	prepare_rogue36_shared_state();
	return rogue36_main(argc, argv, envp);
    }
    if (rogue_variant_is_current("srogue90"))
    {
	return srogue90_main(argc, argv, envp);
    }

#ifdef MASTER
    /*
     * Check to see if he is a wizard
     */
    if (argc >= 2 && argv[1][0] == '\0')
	if (strcmp(PASSWD, md_crypt(md_getpass("wizard's password: "), "mT")) == 0)
	{
	    wizard = TRUE;
	    player.t_flags |= SEEMONST;
	    argv++;
	    argc--;
	}

#endif

    /*
     * get home and options from environment
     */

    strncpy(home, md_gethomedir(), MAXSTR);

    strcpy(file_name, home);
    strcat(file_name, "rogue.save");

    if ((env = getenv("ROGUEOPTS")) != NULL)
	parse_opts(env);
    if (env == NULL || whoami[0] == '\0')
        strucpy(whoami, md_getusername(), (int) strlen(md_getusername()));
    lowtime = (int) time(NULL);
#ifdef MASTER
    if (wizard && getenv("SEED") != NULL)
	dnum = atoi(getenv("SEED"));
    else
#endif
	dnum = lowtime + md_getpid();
    seed = dnum;

    open_score();

	/* 
     * Drop setuid/setgid after opening the scoreboard file. 
     */ 

    md_normaluser();

    /*
     * check for print-score option
     */

	md_normaluser(); /* we drop any setgid/setuid priveldges here */

    if (argc == 2)
    {
	if (strcmp(argv[1], "-s") == 0)
	{
	    if (rogue_frontend_is_tiles() && !rogue_frontend_start())
		my_exit(1);
	    noscore = TRUE;
	    score(0, -1, 0);
	    exit(0);
	}
	else if (strcmp(argv[1], "-d") == 0)
	{
	    dnum = rnd(100);	/* throw away some rnd()s to break patterns */
	    while (--dnum)
		rnd(100);
	    purse = rnd(100) + 1;
	    level = rnd(100) + 1;
	    initscr();
	    getltchars();
	    if (rogue_frontend_is_tiles() && !rogue_frontend_start())
		my_exit(1);
	    death(death_monst());
	    exit(0);
	}
    }

    init_check();			/* check for legal startup */
    if (argc == 2)
	if (!restore(argv[1], envp))	/* Note: restore will never return */
	    my_exit(1);
#ifdef MASTER
    if (wizard)
    {
	if (!rogue_frontend_is_tiles())
	    printf("Hello %s, welcome to dungeon #%d", whoami, dnum);
    }
    else
#endif
    {
	if (!rogue_frontend_is_tiles())
	    printf("Hello %s, just a moment while I dig the dungeon...", whoami);
    }
    if (!rogue_frontend_is_tiles())
	fflush(stdout);

    initscr();				/* Start up cursor package */
    init_probs();			/* Set up prob tables for objects */
    init_player();			/* Set up initial player stats */
    init_names();			/* Set up names of scrolls */
    init_colors();			/* Set up colors of potions */
    init_stones();			/* Set up stone settings of rings */
    init_materials();			/* Set up materials of wands */
    setup();

    /*
     * The screen must be at least NUMLINES x NUMCOLS
     */
    if (LINES < NUMLINES || COLS < NUMCOLS)
    {
	if (!rogue_frontend_notice(
		"Screen Too Small",
		"Rogue needs a terminal at least 80 columns by 24 rows."))
	    printf("\nSorry, the screen must be at least %dx%d\n",
		   NUMLINES, NUMCOLS);
	endwin();
	my_exit(1);
    }

    /*
     * Set up windows
     */
    hw = newwin(LINES, COLS, 0, 0);
    idlok(stdscr, TRUE);
    idlok(hw, TRUE);
#ifdef MASTER
    noscore = wizard;
#endif
    new_level();			/* Draw current level */
    if (!rogue_frontend_start())
    {
	endwin();
	my_exit(1);
    }
    rogue_frontend_render();
    if (rogue_frontend_smoke_requested())
	my_exit(0);
    /*
     * Start up daemons and fuses
     */
    start_daemon(runners, 0, AFTER);
    start_daemon(doctor, 0, AFTER);
    fuse(swander, 0, WANDERTIME, AFTER);
    start_daemon(stomach, 0, AFTER);
    playit();
    return(0);
}

/*
 * endit:
 *	Exit the program abnormally.
 */

void
endit(int sig)
{
    NOOP(sig);
    fatal("Okay, bye bye!\n");
}

/*
 * fatal:
 *	Exit the program, printing a message.
 */

void
fatal(char *s)
{
    mvaddstr(LINES - 2, 0, s);
    refresh();
    endwin();
    my_exit(0);
}

/*
 * rnd:
 *	Pick a very random number.
 */
int
rnd(int range)
{
    return range == 0 ? 0 : abs((int) RN) % range;
}

/*
 * roll:
 *	Roll a number of dice
 */
int 
roll(int number, int sides)
{
    int dtotal = 0;

    while (number--)
	dtotal += rnd(sides)+1;
    return dtotal;
}

/*
 * tstp:
 *	Handle stop and start signals
 */

void
tstp(int ignored)
{
    int y, x;
    int oy, ox;

	NOOP(ignored);

    /*
     * leave nicely
     */
    getyx(curscr, oy, ox);
    mvcur(0, COLS - 1, LINES - 1, 0);
    endwin();
    resetltchars();
    fflush(stdout);
	md_tstpsignal();

    /*
     * start back up again
     */
	md_tstpresume();
    raw();
    noecho();
    keypad(stdscr,1);
    playltchars();
    clearok(curscr, TRUE);
    wrefresh(curscr);
    getyx(curscr, y, x);
    mvcur(y, x, oy, ox);
    fflush(stdout);
    wmove(curscr, oy, ox);
}

/*
 * playit:
 *	The main loop of the program.  Loop until the game is over,
 *	refreshing things and looking at the proper times.
 */

void
playit()
{
    char *opts;

    /*
     * set up defaults for slow terminals
     */

    if (baudrate() <= 1200)
    {
	terse = TRUE;
	jump = TRUE;
	see_floor = FALSE;
    }

    if (md_hasclreol())
	inv_type = INV_CLEAR;

    /*
     * parse environment declaration of options
     */
    if ((opts = getenv("ROGUEOPTS")) != NULL)
	parse_opts(opts);


    oldpos = hero;
    oldrp = roomin(&hero);
    while (playing)
	command();			/* Command execution */
    endit(0);
}

/*
 * quit:
 *	Have player make certain, then exit.
 */

void
quit(int sig)
{
    int oy, ox;

    NOOP(sig);

    /*
     * Reset the signal in case we got here via an interrupt
     */
    if (!q_comm)
	mpos = 0;
    getyx(curscr, oy, ox);
    if (rogue_frontend_is_tiles())
    {
	if (!rogue_frontend_confirm("Quit", "Really quit?"))
	    goto no_quit;
    }
    else
    {
	msg("really quit?");
	if (readchar() != 'y')
	    goto no_quit;
    }
    {
	signal(SIGINT, leave);
	clear();
	mvprintw(LINES - 2, 0, "You quit with %d gold pieces", purse);
	move(LINES - 1, 0);
	refresh();
	score(purse, 1, 0);
	my_exit(0);
    }
no_quit:
	move(0, 0);
	clrtoeol();
	status();
	move(oy, ox);
	refresh();
	mpos = 0;
	count = 0;
	to_death = FALSE;
}

/*
 * leave:
 *	Leave quickly, but curteously
 */

void
leave(int sig)
{
    static char buf[BUFSIZ];

    NOOP(sig);

    setbuf(stdout, buf);	/* throw away pending output */

    if (!isendwin())
    {
	mvcur(0, COLS - 1, LINES - 1, 0);
	endwin();
    }

    putchar('\n');
    my_exit(0);
}

/*
 * shell:
 *	Let them escape for a while
 */

void
shell()
{
    if (rogue_frontend_is_tiles())
    {
	msg("shell escape is not available in tile mode");
	after = FALSE;
	return;
    }

    /*
     * Set the terminal back to original mode
     */
    move(LINES-1, 0);
    refresh();
    endwin();
    resetltchars();
    putchar('\n');
    in_shell = TRUE;
    after = FALSE;
    fflush(stdout);
    /*
     * Fork and do a shell
     */
    md_shellescape();

    printf("\n[Press return to continue]");
    fflush(stdout);
    noecho();
    raw();
    keypad(stdscr,1);
    playltchars();
    in_shell = FALSE;
    wait_for('\n');
    clearok(stdscr, TRUE);
}

/*
 * my_exit:
 *	Leave the process properly
 */

void
my_exit(int st)
{
    rogue_frontend_shutdown();
    resetltchars();
    exit(st);
}

