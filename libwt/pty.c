#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include "pty.h"

int openpty(struct pty *pty)
{
    int _;

    pty->child_pid = 0;

    pty->ptm = posix_openpt(O_RDWR);
    if (pty->ptm < 0)
    {
        perror("posix_openpt");
        goto fail;
    }

    _ = grantpt(pty->ptm);
    if (_ < 0)
    {
        perror("grantpt");
        goto close_ptm;
    }

    _ = unlockpt(pty->ptm);
    if (_ < 0)
    {
        perror("unlockpt");
        goto close_ptm;
    }

    pty->pts = open(ptsname(pty->ptm), O_RDWR);
    if (pty->pts < 0)
    {
        perror("open");
        goto close_ptm;
    }

    return 0;

close_ptm:
    close(pty->ptm);
fail:
    return -1;
}

int forkpty(struct pty *pty)
{
    int _;

    _ = openpty(pty);
    if (_ < 0)
        goto fail;

    pty->child_pid = fork();
    if (pty->child_pid == 0)
    {
        close(pty->ptm);
        pty->ptm = -1;

        _ = dup2(pty->pts, STDIN_FILENO);
        if (_ < 0)
        {
            perror("dup2");
            goto child_fail;
        }
        _ = dup2(pty->pts, STDOUT_FILENO);
        if (_ < 0)
        {
            perror("dup2");
            goto child_fail;
        }
        _ = dup2(pty->pts, STDERR_FILENO);
        if (_ < 0)
        {
            perror("dup2");
            goto child_fail;
        }

        close(pty->pts);

        setsid();

        _ = ioctl(STDIN_FILENO, TIOCSCTTY, 1);
        if (_ < 0)
        {
            perror("TIOCSCTTY");
            goto child_fail;
        }

        return 0;

child_fail:
        exit(EXIT_FAILURE);
    }
    else if (pty->child_pid > 0)
    {
        close(pty->pts);
        pty->pts = -1;
        return 1;
    }
    else
    {
        perror("fork");
        goto close_pty;
    }

close_pty:
    closepty(pty);
fail:
    return -1;
}

void closepty(struct pty *pty)
{
    if (pty->ptm >= 0)
    {
        close(pty->ptm);
        pty->ptm = -1;
    }

    if (pty->pts >= 0)
    {
        close(pty->pts);
        pty->pts = -1;
    }
}