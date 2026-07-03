/*
 * save and restore routines
 *
 * @(#)save.c	4.33 (Berkeley) 06/01/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <curses.h>
#include "rogue.h"
#include "frontend.h"
#include "score.h"

typedef struct stat STAT;

extern char version[], encstr[];

static STAT sbuf;

static bool
restore_error(char *message)
{
    if (!rogue_frontend_notice("Restore Failed", message))
	printf("%s\n", message);
    return FALSE;
}

/*
 * save_game:
 *	Implement the "save game" command
 */

void
save_game()
{
    FILE *savef;
    int c;
    auto char buf[MAXSTR];

    /*
     * get file name
     */
    mpos = 0;
over:
    if (file_name[0] != '\0')
    {
	for (;;)
	{
	    if (rogue_frontend_is_tiles())
	    {
		snprintf(buf, sizeof(buf), "Save file (%s)?", file_name);
		c = rogue_frontend_confirm("Save Game", buf) ? 'y' : 'n';
		break;
	    }
	    else
	    {
		msg("save file (%s)? ", file_name);
		c = readchar();
		mpos = 0;
		if (c == ESCAPE)
		{
		    msg("");
		    return;
		}
		else if (c == 'n' || c == 'N' || c == 'y' || c == 'Y')
		    break;
		else
		    msg("please answer Y or N");
	    }
	}
	if (c == 'y' || c == 'Y')
	{
	    if (!rogue_frontend_is_tiles())
	    {
		addstr("Yes\n");
		refresh();
	    }
	    strcpy(buf, file_name);
	    goto gotfile;
	}
    }

    do
    {
	mpos = 0;
	msg("file name: ");
	buf[0] = '\0';
	if (get_str(buf, stdscr) == QUIT)
	{
quit_it:
	    msg("");
	    return;
	}
	mpos = 0;
gotfile:
	/*
	 * test to see if the file exists
	 */
	if (stat(buf, &sbuf) >= 0)
	{
	    for (;;)
	    {
		if (rogue_frontend_is_tiles())
		{
		    c = rogue_frontend_confirm(
			"Overwrite Save",
			"File exists. Do you wish to overwrite it?")
			? 'y' : 'n';
		    break;
		}
		else
		{
		    msg("File exists.  Do you wish to overwrite it?");
		    mpos = 0;
		    if ((c = readchar()) == ESCAPE)
			goto quit_it;
		    if (c == 'y' || c == 'Y')
			break;
		    else if (c == 'n' || c == 'N')
			goto over;
		    else
			msg("Please answer Y or N");
		}
	    }
	    if (c == 'n' || c == 'N')
		goto over;
	    msg("file name: %s", buf);
	    md_unlink(file_name);
	}
	strcpy(file_name, buf);
	if ((savef = fopen(file_name, "w")) == NULL)
	    msg(strerror(errno));
    } while (savef == NULL);

    save_file(savef);
    /* NOTREACHED */
}

/*
 * auto_save:
 *	Automatically save a file.  This is used if a HUP signal is
 *	recieved
 */

void
auto_save(int sig)
{
    FILE *savef;
    NOOP(sig);

    md_ignoreallsignals();
    if (file_name[0] != '\0' && ((savef = fopen(file_name, "w")) != NULL ||
	(md_unlink_open_file(file_name, savef) >= 0 && (savef = fopen(file_name, "w")) != NULL)))
	    save_file(savef);
    exit(0);
}

/*
 * save_file:
 *	Write the saved game on the file
 */

void
save_file(FILE *savef)
{
    char buf[80];
    mvcur(0, COLS - 1, LINES - 1, 0); 
    putchar('\n');
    endwin();
    resetltchars();
    md_chmod(file_name, 0400);
    encwrite(version, strlen(version)+1, savef);
    sprintf(buf,"%d x %d\n", LINES, COLS);
    encwrite(buf,80,savef);
    rs_save_file(savef);
    fflush(savef);
    fclose(savef);
    exit(0);
}

/*
 * restore:
 *	Restore a saved game from a file with elaborate checks for file
 *	integrity from cheaters
 */
bool
restore(char *file, char **envp)
{
    FILE *inf;
    int syml;
    extern char **environ;
    auto char buf[MAXSTR];
    auto STAT sbuf2;
    int lines, cols;

    if (strcmp(file, "-r") == 0)
	file = file_name;

	md_tstphold();

	if ((inf = fopen(file,"r")) == NULL)
    {
	snprintf(buf, sizeof(buf), "%s: %s", file, strerror(errno));
	return restore_error(buf);
    }
    stat(file, &sbuf2);
    syml = is_symlink(file);

    fflush(stdout);
    encread(buf, (unsigned) strlen(version) + 1, inf);
    if (strcmp(buf, version) != 0)
    {
	return restore_error("Sorry, saved game is out of date.");
    }
    encread(buf,80,inf);
    sscanf(buf,"%d x %d\n", &lines, &cols);

    initscr();                          /* Start up cursor package */
    keypad(stdscr, 1);

    if (lines > LINES)
    {
        endwin();
	snprintf(buf, sizeof(buf),
		 "Saved game needs %d screen lines; current screen has %d.",
		 lines, LINES);
        return restore_error(buf);
    }
    if (cols > COLS)
    {
        endwin();
	snprintf(buf, sizeof(buf),
		 "Saved game needs %d screen columns; current screen has %d.",
		 cols, COLS);
        return restore_error(buf);
    }

    hw = newwin(LINES, COLS, 0, 0);
    setup();

    rs_restore_file(inf);
    /*
     * we do not close the file so that we will have a hold of the
     * inode for as long as possible
     */

    if (
#ifdef MASTER
	!wizard &&
#endif
        md_unlink_open_file(file, inf) < 0)
    {
	return restore_error("Cannot unlink file.");
    }
    mpos = 0;
/*    printw(0, 0, "%s: %s", file, ctime(&sbuf2.st_mtime)); */
/*
    printw("%s: %s", file, ctime(&sbuf2.st_mtime));
*/
    clearok(stdscr,TRUE);
    /*
     * defeat multiple restarting from the same place
     */
#ifdef MASTER
    if (!wizard)
#endif
	if (sbuf2.st_nlink != 1 || syml)
	{
	    endwin();
	    return restore_error("Cannot restore from a linked file.");
	}

    if (pstats.s_hpt <= 0)
    {
	endwin();
	return restore_error("\"He's dead, Jim\"");
    }

	md_tstpresume();

    environ = envp;
    strcpy(file_name, file);
    clearok(curscr, TRUE);
    srand(md_getpid());
    msg("file name: %s", file);
    if (!rogue_frontend_start())
    {
	endwin();
	return restore_error("Could not start tile frontend.");
    }
    rogue_frontend_render();
    if (rogue_frontend_smoke_requested())
	my_exit(0);
    playit();
    /*NOTREACHED*/
    return(0);
}

/*
 * encwrite:
 *	Perform an encrypted write
 */

size_t
encwrite(char *start, size_t size, FILE *outf)
{
    char *e1, *e2, fb;
    int temp;
    extern char statlist[];
    size_t o_size = size;
    e1 = encstr;
    e2 = statlist;
    fb = 0;

    while(size)
    {
	if (putc(*start++ ^ *e1 ^ *e2 ^ fb, outf) == EOF)
            break;

	temp = *e1++;
	fb = fb + ((char) (temp * *e2++));
	if (*e1 == '\0')
	    e1 = encstr;
	if (*e2 == '\0')
	    e2 = statlist;
	size--;
    }

    return(o_size - size);
}

/*
 * encread:
 *	Perform an encrypted read
 */
size_t
encread(char *start, size_t size, FILE *inf)
{
    char *e1, *e2, fb;
    int temp;
    size_t read_size;
    extern char statlist[];

    fb = 0;

    if ((read_size = fread(start,1,size,inf)) == 0 || read_size == -1)
	return(read_size);

    e1 = encstr;
    e2 = statlist;

    while (size--)
    {
	*start++ ^= *e1 ^ *e2 ^ fb;
	temp = *e1++;
	fb = fb + (char)(temp * *e2++);
	if (*e1 == '\0')
	    e1 = encstr;
	if (*e2 == '\0')
	    e2 = statlist;
    }

    return(read_size);
}

static char scoreline[100];
/*
 * read_scrore
 *	Read in the score file
 */
void
rd_score(SCORE *top_ten)
{
    unsigned int i;

	if (scoreboard == NULL)
		return;

	rewind(scoreboard); 

	for(i = 0; i < numscores; i++)
    {
        encread(top_ten[i].sc_name, MAXSTR, scoreboard);
        encread(scoreline, 100, scoreboard);
        sscanf(scoreline, " %u %d %u %hu %d %x \n",
            &top_ten[i].sc_uid, &top_ten[i].sc_score,
            &top_ten[i].sc_flags, &top_ten[i].sc_monster,
            &top_ten[i].sc_level, &top_ten[i].sc_time);
    }

	rewind(scoreboard); 
}

/*
 * write_scrore
 *	Read in the score file
 */
void
wr_score(SCORE *top_ten)
{
    unsigned int i;

	if (scoreboard == NULL)
		return;

	rewind(scoreboard);

    for(i = 0; i < numscores; i++)
    {
          memset(scoreline,0,100);
          encwrite(top_ten[i].sc_name, MAXSTR, scoreboard);
          sprintf(scoreline, " %u %d %u %hu %d %x \n",
              top_ten[i].sc_uid, top_ten[i].sc_score,
              top_ten[i].sc_flags, top_ten[i].sc_monster,
              top_ten[i].sc_level, top_ten[i].sc_time);
          encwrite(scoreline,100,scoreboard);
    }

	rewind(scoreboard); 
}
