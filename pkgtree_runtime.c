#define PKGTREE_RUNTIME_IMPL
#include "pkgtree_runtime.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>

static char repo_url[1024] = PKGTREE_DEFAULT_REPO;

static int mkdir_p(const char *path) {
    if (!path || !*path) return 0;
    char tmp[PATH_MAX];
    size_t len = strlen(path);
    if (len >= sizeof(tmp)) return -1;
    strcpy(tmp, path);
    if (tmp[len-1] == '/') tmp[len-1] = '\0';
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
                *p = '/';
                return -1;
            }
            *p = '/';
        }
    }
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return -1;
    return 0;
}

static int file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

static int write_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    if (content) fputs(content, f);
    fclose(f);
    return 0;
}

static int append_line(const char *path, const char *line) {
    FILE *f = fopen(path, "a");
    if (!f) return -1;
    if (line) fputs(line, f);
    fputs("\n", f);
    fclose(f);
    return 0;
}

static int try_fetch(const char *url, const char *outpath) {
    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "curl -fsSL -o '%s' '%s' 2>/dev/null", outpath, url);
    if (system(cmd) == 0) return 0;
    snprintf(cmd, sizeof(cmd), "wget -q -O '%s' '%s' 2>/dev/null", outpath, url);
    if (system(cmd) == 0) return 0;
    return -1;
}

int pkgtree_init(void) {
    mkdir_p(".tree");
    mkdir_p(".tree/cache");
    mkdir_p(".tree/packages");
    if (!file_exists("tree.pkg")) {
        char buf[1024];
        snprintf(buf, sizeof(buf), "repo=\"%s\"\n", repo_url);
        write_file("tree.pkg", buf);
    }
    if (!file_exists("tree.lock")) {
        write_file("tree.lock", "{locked=[]}\n");
    }
    printf("[pkgtree][ok] initialized tree.pkg and tree.lock\n");
    return 1;
}

int pkgtree_set_repo(const char* url) {
    if (!url) return 0;
    strncpy(repo_url, url, sizeof(repo_url)-1);
    repo_url[sizeof(repo_url)-1] = '\0';
    printf("[pkgtree] repository set: %s\n", repo_url);
    return 1;
}

int pkgtree_add(const char* name, const char* version) {
    if (!name || !version) return 0;
    char line[512];
    snprintf(line, sizeof(line), "%s,%s", name, version);
    if (append_line("tree.pkg", line) != 0) {
        fprintf(stderr, "[pkgtree][err] failed to append to tree.pkg\n");
        return 0;
    }
    printf("[pkgtree][ok] added dependency %s@%s\n", name, version);
    return 1;
}

int pkgtree_list(void) {
    FILE *f = fopen("tree.pkg", "r");
    if (!f) { printf("[pkgtree] no tree.pkg found\n"); return 0; }
    char buf[1024];
    int count = 0;
    while (fgets(buf, sizeof(buf), f)) {
        char *p = strchr(buf, '\n'); if (p) *p = '\0';
        if (strlen(buf) == 0) continue;
        printf(" - %s\n", buf);
        count++;
    }
    fclose(f);
    printf("[pkgtree] dependencies: %d\n", count);
    return count;
}

int pkgtree_install(void) {
    mkdir_p(".tree");
    mkdir_p(".tree/cache");
    mkdir_p(".tree/packages");
    if (!file_exists("tree.pkg")) { fprintf(stderr, "[pkgtree][err] no tree.pkg found\n"); return 0; }
    FILE *f = fopen("tree.pkg","r");
    if (!f) return 0;
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        char tmp[1024];
        strncpy(tmp, line, sizeof(tmp)-1);
        tmp[sizeof(tmp)-1] = '\0';
        char *nl = strchr(tmp, '\n'); if (nl) *nl = '\0';
        if (strlen(tmp) == 0) continue;
        char *comma = strchr(tmp, ',');
        if (!comma) continue;
        *comma = '\0';
        char *name = tmp;
        char *ver = comma + 1;
        char pkgdir[PATH_MAX];
        snprintf(pkgdir, sizeof(pkgdir), ".tree/packages/%s/%s", name, ver);
        mkdir_p(pkgdir);
        char url[1024];
        snprintf(url, sizeof(url), "%s/%s/%s/package.tree", repo_url, name, ver);
        char outpath[PATH_MAX];
        snprintf(outpath, sizeof(outpath), ".tree/cache/%s-%s.tree", name, ver);
        if (try_fetch(url, outpath) == 0) {
            printf("[pkgtree] fetched %s@%s -> %s\n", name, ver, outpath);
        } else {
            printf("[pkgtree] failed to fetch %s@%s\n", name, ver);
        }
    }
    fclose(f);
    write_file("tree.lock", "{locked=\"ok\"}\n");
    printf("[pkgtree][ok] install complete\n");
    return 1;
}

int pkgtree_update(const char* name) {
    if (!name) return 0;
    if (!file_exists("tree.pkg")) { fprintf(stderr, "[pkgtree][err] no tree.pkg\n"); return 0; }
    FILE *f = fopen("tree.pkg","r");
    if (!f) return 0;
    /* create temp file in current directory to avoid relying on /tmp */
    char tmpfile[] = "tree.pkg.tmpXXXXXX";
    int fd = mkstemp(tmpfile);
    if (fd < 0) { fclose(f); return 0; }
    FILE *out = fdopen(fd, "w");
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        char copy[1024];
        strncpy(copy, line, sizeof(copy)-1);
        copy[sizeof(copy)-1] = '\0';
        char *nl = strchr(copy, '\n'); if (nl) *nl = '\0';
        char *comma = strchr(copy, ',');
        if (comma) *comma = '\0';
        if (strcmp(copy, name) == 0) {
            fprintf(out, "%s,%s\n", name, "latest");
        } else {
            fputs(line, out);
        }
    }
    fclose(f);
    fclose(out);
    rename(tmpfile, "tree.pkg");
    printf("[pkgtree][ok] updated %s\n", name);
    return 1;
}

int pkgtree_pkg_search(const char* pkg_name, const char* alias) {
    if (!pkg_name) return 0;
    const char *libdir = "/usr/lib";
    DIR *d = opendir(libdir);
    if (!d) {
        fprintf(stderr, "[pkgtree][err] cannot open %s\n", libdir);
        return 0;
    }
    struct dirent *entry;
    int found = 0;
    while ((entry = readdir(d)) != NULL) {
        if (strncmp(entry->d_name, "pks", 3) == 0) {
            char candidate[PATH_MAX];
            snprintf(candidate, sizeof(candidate), "%s/%s/%s.tree", libdir, entry->d_name, pkg_name);
            if (access(candidate, R_OK) == 0) {
                printf("[pkgtree] found package file: %s (as %s)\n", candidate, alias?alias:pkg_name);
                found++;
            } else {
                snprintf(candidate, sizeof(candidate), "%s/%s/%s", libdir, entry->d_name, pkg_name);
                if (access(candidate, R_OK) == 0) {
                    printf("[pkgtree] found package dir: %s (as %s)\n", candidate, alias?alias:pkg_name);
                    found++;
                }
            }
        }
    }
    closedir(d);
    if (found == 0) fprintf(stderr, "[pkgtree] package %s not found in /usr/lib/pks*\n", pkg_name);
    return found;
}
