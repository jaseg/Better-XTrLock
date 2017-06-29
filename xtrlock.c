/*
 * xtrlock.c
 *
 * X Transparent Lock
 *
 * Copyright (C)1993,1994 Ian Jackson
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <crypt.h>
#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <limits.h>
#include <math.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <values.h>

#ifdef SHADOW_PWD
#include <shadow.h>
#endif

//#define DEBUG
#ifdef DEBUG
#define debug_print(...)             \
        do {                         \
                printf(__VA_ARGS__); \
        } while (0)
#else
#define debug_print(...) \
        do {             \
        } while (0)
#endif

#define msleep(x) usleep(x * 1000)

Display* display;
Window window, blank_window, trans_window;

#define TIMEOUTPERATTEMPT 30000
#define MAXGOODWILL (TIMEOUTPERATTEMPT * 5)
#define INITIALGOODWILL MAXGOODWILL
#define GOODWILLPORTION 0.3
bool blank_screen = false;
int blink_delay = 100;

struct { /*Setting correspond to the custom passwd setting. --d0048*/
        bool enable;
        char* pwd;
} cust_pw_setting;

struct passwd* pw;
int passwordok(const char* s)
{
        /* simpler, and should work with crypt() algorithms using longer
           salt strings (like the md5-based one on freebsd).  --marekm */
        debug_print("%s, %i\n", s, (int)strlen(s));

        if (cust_pw_setting.enable) {
                debug_print("Entered_de: %s\n", s);
                debug_print("Original_de: %s\n", cust_pw_setting.pwd);
                char* enter = crypt(s, cust_pw_setting.pwd);
                char* original = cust_pw_setting.pwd;
                if (NULL == enter) {
                        fprintf(stderr, "\"strdup\" or \"crypt\":%s\n", strerror(errno));
                        return false;
                }
                debug_print("Entered:  %s\n", enter);
                debug_print("Original: %s\n", original);
                return !strcmp(enter, original);
        } else { /*salt in the second argument seems to be automatically used by crypt*/
                char* result = crypt(s, pw->pw_passwd);
                if (NULL == result) {
                        fprintf(stderr, "\"strdup\" or \"crypt\":%s\n", strerror(errno));
                        return false;
                }
                debug_print("Entered_de:  %s\n", result);
                debug_print("Original_de: %s\n", pw->pw_passwd);
                return !strcmp(result, pw->pw_passwd);
        }
}

void print_help()
{
        printf("Xtrlock:\n"
                        "    -h                      show this help\n"
                        "    -l                      lock immediately with user's default password\n"
                        "    -p [password_string]    use custom non-encrypted password\n"
                        "    -e [password_hash]      use encrypted custom password with salt of itself\n"
                        "    -c [password_string]    calculate the password string that can be used with the \"-c\" option\n"
                        "    -b                      lock with a blank screen\n"
                        "    -d [delay_usec]         milliseconds the screen blinks on successful locks(0 for no-delay & 100000 for 0.1 s)\n"
                        "Thanks for using!\n");
}

char rand_ch()
{ /*range of characters in int: [A-Z]|[a-z][0-9](rand % max-min+1+min)*/
        /*time based rand refreshes too slow thus not the best choice*/
        /*use a-z in this case only*/
        return (char)((rand() % (90 - 65 + 1)) + 65);
}

int lock()
{
        XEvent ev;
        KeySym ks;
        char cbuf[10], rbuf[128]; /* shadow appears to suggest 127 a good value here */
        int clen, rlen = 0;
        long goodwill = INITIALGOODWILL, timeout = 0;
        XSetWindowAttributes attrib;
        Cursor cursor;
        Pixmap csr;
        XColor xcolor;
        int ret;
        static char csr_bits[] = { 0x00 };
        struct timeval tv;
        int tvt, gs;

        if (!cust_pw_setting.enable) {
                debug_print("Sensing user pwd\n");
#ifdef SHADOW_PWD
                struct spwd* sp;
#endif
                errno = 0;
                pw = getpwuid(getuid());
                if (!pw) {
                        perror("password entry for uid not found");
                        exit(1);
                }
#ifdef SHADOW_PWD
                sp = getspnam(pw->pw_name);
                if (sp)
                        pw->pw_passwd = sp->sp_pwdp;
                endspent();
#endif

                /* logically, if we need to do the following then the same
                   applies to being installed setgid shadow.
                   we do this first, because of a bug in linux. --jdamery */
                setgid(getgid());
                /* we can be installed setuid root to support shadow passwords,
                   and we don't need root privileges any longer.  --marekm */
                setuid(getuid());

                if (strlen(pw->pw_passwd) <= 1) { /*mark as 'x', which means shadow password is enabled.*/
                        fputs("password entry has no pwd\n", stderr);
                        exit(1);
                }
                debug_print("Passwd sensed:%s\n", pw->pw_passwd);
        }
        display = XOpenDisplay(0);

        if (display == NULL) {
                fprintf(stderr, "xtrlock: cannot open display\n");
                exit(1);
        }

        attrib.override_redirect = True;
        attrib.background_pixel = BlackPixel(display, DefaultScreen(display));

        blank_window = XCreateWindow(display, DefaultRootWindow(display), /*init blank window*/
                        0, 0, DisplayWidth(display, DefaultScreen(display)),
                        DisplayHeight(display, DefaultScreen(display)),
                        0, DefaultDepth(display, DefaultScreen(display)), CopyFromParent, DefaultVisual(display, DefaultScreen(display)),
                        CWOverrideRedirect | CWBackPixel, &attrib);

        trans_window = XCreateWindow(display, DefaultRootWindow(display), /*init window identical to Background*/
                        0, 0, 1, 1, 0, CopyFromParent, InputOnly, CopyFromParent,
                        CWOverrideRedirect, &attrib);

        window = trans_window;

        XSelectInput(display, window, KeyPressMask | KeyReleaseMask);

        csr = XCreateBitmapFromData(display, window, csr_bits, 1, 1);

        cursor = XCreatePixmapCursor(display, csr, csr, &xcolor, &xcolor, 1, 1);

        XMapWindow(display, window);

        /*Sometimes the WM doesn't ungrab the keyboard quickly enough if
         *launching xtrlock from a keystroke shortcut, meaning xtrlock fails
         *to start We deal with this by waiting (up to 100 times) for 10,000
         *microsecs and trying to grab each time. If we still fail
         *(i.e. after 1s in total), then give up, and emit an error.
         *Also, only blank the screen to indicate a success lock.
         */

        gs = 0; /*gs==grab successful*/
        for (tvt = 0; tvt < 100; tvt++) {
                ret = XGrabKeyboard(display, window, False, GrabModeAsync, GrabModeAsync,
                                CurrentTime);
                if (ret == GrabSuccess) {
                        gs = 1;
                        break;
                }
                /*grab failed; wait .01s*/
                tv.tv_sec = 0;
                tv.tv_usec = 10000;
                select(1, NULL, NULL, NULL, &tv);
        }
        if (gs == 0) {
                fprintf(stderr, "xtrlock: cannot grab keyboard\n");
                exit(1);
        }

        if (XGrabPointer(display, window, False, (KeyPressMask | KeyReleaseMask) & 0,
                                GrabModeAsync, GrabModeAsync, None,
                                cursor, CurrentTime)
                        != GrabSuccess) {
                XUngrabKeyboard(display, CurrentTime);
                fprintf(stderr, "xtrlock: cannot grab pointer\n");
                exit(1);
        }

        if (!blank_screen && blink_delay != 0) { /*blink to indicate a successful lock*/
                XMapWindow(display, blank_window);
                XFlush(display);
                msleep(blink_delay); /*0.1s as default, or custom value*/
                XUnmapWindow(display, blank_window);
                debug_print("Unmapped after %i u seconds\n", blink_delay);
                XMapWindow(display, trans_window);
                XFlush(display);
                debug_print("locked and blinked\n");
        }

        if(blank_screen){
                XMapWindow(display, blank_window);
        }

        printf("Successfully locked\n");

        for (;;) { /*start checker loop*/
                XNextEvent(display, &ev);
                switch (ev.type) {
                        case KeyPress:
                                if (ev.xkey.time < timeout) {
                                        XBell(display, 0);
                                        break;
                                }
                                clen = XLookupString(&ev.xkey, cbuf, 9, &ks, 0);
                                switch (ks) {
                                        case XK_Escape:
                                        case XK_Clear:
                                                rlen = 0;
                                                break;
                                        case XK_Delete:
                                        case XK_BackSpace:
                                                if (rlen > 0)
                                                        rlen--;
                                                break;
                                        case XK_Linefeed:
                                        case XK_Return:
                                                if (rlen == 0)
                                                        break;
                                                else
                                                        rbuf[rlen] = 0;
                                                if (passwordok(rbuf))
                                                        goto loop_x;
                                                XBell(display, 0);
                                                rlen = 0;
                                                if (timeout) {
                                                        goodwill += ev.xkey.time - timeout;
                                                        if (goodwill > MAXGOODWILL) {
                                                                goodwill = MAXGOODWILL;
                                                        }
                                                }
                                                timeout = -goodwill * GOODWILLPORTION;
                                                goodwill += timeout;
                                                timeout += ev.xkey.time + TIMEOUTPERATTEMPT;
                                                break;
                                        default:
                                                if (clen != 1)
                                                        break;
                                                /* allow space for the trailing \0 */
                                                if (rlen < (sizeof(rbuf) - 1)) {
                                                        rbuf[rlen] = cbuf[0];
                                                        rlen++;
                                                }
                                                break;
                                }
                                break;

                        default:
                                break;
                }
        } /*end checker loop*/
loop_x:   /*loop exit*/
        exit(0);
}

int main(int argc, char** argv)
{ /*TODO: add keeper process*/
        errno = 0;
        bool need_lock = false;
        cust_pw_setting.enable = false;

        FILE* fp_dev_rand = NULL;
        fp_dev_rand = fopen("/dev/urandom", "r");
        if (!fp_dev_rand) {
                fprintf(stderr, "failed to open /dev/random: %s\n", strerror(errno));
                exit(1);
        }
        unsigned int seed;
        fread(&seed, sizeof(int), 1, fp_dev_rand);
        //fscanf(fp_dev_rand, "%u", &seed);
        debug_print("Read seed from /dev/rand: %u\n", seed);
        srand(seed);
        fclose(fp_dev_rand);

        int i = 10;
        while (i-- > 0)
                debug_print("rand%i:%c\n", 10 - i, rand_ch());
        char opt = 0;
        while ((opt = getopt(argc, argv, "hp:e:c:lbd:")) != -1) {
                //while ((opt = getopt(argc, argv, ":h:p:e:c:l:b:d:")) != -1) {
                debug_print("Processing args: \"%c|%c\"\n", opt, optopt);

                if ('h' == opt) { /*help(no arg)*/
                        print_help();
                        exit(0);
                }
                if ('p' == opt) { /*custom pwd without encryption*/
                        char f_salt[3] = { rand_ch(), rand_ch(), '\0' };
                        debug_print("salt_generated: %s\n", f_salt);
                        cust_pw_setting.enable = true;
                        cust_pw_setting.pwd = strdup(crypt(optarg, f_salt)); /*never freed, fine in this case*/
                        need_lock = true;
                        if (NULL == cust_pw_setting.pwd) {
                                fprintf(stderr, "strdup:%s\n", strerror(errno));
                                exit(-1);
                        }
                }
                if ('e' == opt) { /*custom pwd encrypted already*/
                        cust_pw_setting.enable = true;
                        cust_pw_setting.pwd = optarg;
                        need_lock = true;
                }
                if ('c' == opt) { /*encryption of pwd*/
                        char f_salt[2] = { rand_ch(), rand_ch() };
                        printf("%s\n", crypt(optarg, f_salt));
                        exit(0);
                }
                if ('l' == opt) { /*lock with user default password after delay(no arg)*/
                        debug_print("locked immidiently\n");
                        need_lock = true;
                }
                if ('b' == opt) { /*lock with a blank screen(no arg)*/
                        blank_screen = true;
                        //need_lock = true;
                        debug_print("blank_screen mode \n");
                }
                if ('d' == opt) { /*delay of screen blinks*/
                        blink_delay = atoi(optarg);
                        if (blink_delay < 0) {
                                fprintf(stderr, "Delay value not valid\n");
                                exit(1);
                        }
                }
        }
        if (need_lock)
                lock();
        print_help();
        exit(1);
        }
