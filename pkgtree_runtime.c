#define PKGTREE_RUNTIME_IMPL
#include "pkgtree_runtime.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>

static char repo_url[1024] = PKGTREE_DEFAULT_REPO;

static char *fmt_alloc(const char *fmt, ...) {
    va_list ap;
    va_list copy;
    int needed;
    char *out;

    va_start(ap, fmt);
    va_copy(copy, ap);
    needed = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);
    if (needed < 0) {
        va_end(ap);
        return NULL;
    }
    out = (char *)malloc((size_t)needed + 1);
    if (!out) {
        va_end(ap);
        return NULL;
    }
    vsnprintf(out, (size_t)needed + 1, fmt, ap);
    va_end(ap);
    return out;
}

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

static void trim_in_place(char *s) {
    char *start;
    char *end;
    size_t len;
    if (!s) return;
    start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    len = strlen(s);
    if (len == 0) return;
    end = s + len - 1;
    while (end >= s && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
}

static int starts_with(const char *s, const char *prefix) {
    size_t n;
    if (!s || !prefix) return 0;
    n = strlen(prefix);
    return strncmp(s, prefix, n) == 0;
}

static int copy_file(const char *src, const char *dst) {
    FILE *in;
    FILE *out;
    char buf[4096];
    size_t n;
    in = fopen(src, "rb");
    if (!in) return -1;
    out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return -1;
    }
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            fclose(in);
            fclose(out);
            return -1;
        }
    }
    fclose(in);
    fclose(out);
    return 0;
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
    char *cmd = fmt_alloc("curl -fsSL -o '%s' '%s' 2>/dev/null", outpath, url);
    if (cmd) {
        if (system(cmd) == 0) { free(cmd); return 0; }
        free(cmd);
    }
    cmd = fmt_alloc("wget -q -O '%s' '%s' 2>/dev/null", outpath, url);
    if (cmd) {
        if (system(cmd) == 0) { free(cmd); return 0; }
        free(cmd);
    }
    return -1;
}

static int repo_is_local(const char *repo) {
    if (!repo || !*repo) return 0;
    return strstr(repo, "://") == NULL;
}

static char *local_repo_manifest_path(void) {
    return fmt_alloc("%s/MANIFEST.tree", repo_url);
}

static char *local_repo_package_path(const char *name, const char *version) {
    return fmt_alloc("%s/pkg/%s@%s.tree", repo_url, name, version);
}

static int parse_dep_line(char *line, char *name_out, size_t name_cap, char *ver_out, size_t ver_cap) {
    char *sep;
    size_t nlen;
    size_t vlen;
    if (!line) return 0;
    trim_in_place(line);
    if (!*line || line[0] == '#') return 0;
    if (strchr(line, '=') && !strchr(line, ',')) return 0;

    sep = strchr(line, ',');
    if (!sep) sep = strchr(line, '@');
    if (!sep) return 0;

    *sep = '\0';
    sep++;
    trim_in_place(line);
    trim_in_place(sep);
    if (!*line || !*sep) return 0;

    nlen = strlen(line);
    if (nlen >= name_cap) nlen = name_cap - 1;
    memcpy(name_out, line, nlen);
    name_out[nlen] = '\0';

    vlen = strlen(sep);
    if (vlen >= ver_cap) vlen = ver_cap - 1;
    memcpy(ver_out, sep, vlen);
    ver_out[vlen] = '\0';
    return 1;
}

static int load_repo_from_tree_pkg(void) {
    FILE *f;
    char line[1024];
    f = fopen("tree.pkg", "r");
    if (!f) return 0;
    while (fgets(line, sizeof(line), f)) {
        trim_in_place(line);
        if (starts_with(line, "repo=")) {
            const char *v = line + 5;
            if (*v) {
                strncpy(repo_url, v, sizeof(repo_url) - 1);
                repo_url[sizeof(repo_url) - 1] = '\0';
            }
        }
    }
    fclose(f);
    return 1;
}

static int local_manifest_find(const char *name, const char *version, char *desc_out, size_t desc_cap, char *deps_out, size_t deps_cap) {
    FILE *f;
    char line[2048];
    char *manifest = local_repo_manifest_path();
    int found = 0;
    if (!manifest) return 0;
    f = fopen(manifest, "r");
    free(manifest);
    if (!f) return 0;

    while (fgets(line, sizeof(line), f)) {
        char *name_f;
        char *ver_f;
        char *desc_f;
        char *deps_f;
        trim_in_place(line);
        if (!*line || line[0] == '#') continue;

        name_f = strtok(line, "\t");
        ver_f = strtok(NULL, "\t");
        desc_f = strtok(NULL, "\t");
        deps_f = strtok(NULL, "\t");
        if (!name_f || !ver_f) continue;
        trim_in_place(name_f);
        trim_in_place(ver_f);
        if (strcmp(name_f, name) != 0 || strcmp(ver_f, version) != 0) continue;

        if (desc_out && desc_cap > 0) {
            if (!desc_f) desc_f = "";
            trim_in_place(desc_f);
            strncpy(desc_out, desc_f, desc_cap - 1);
            desc_out[desc_cap - 1] = '\0';
        }
        if (deps_out && deps_cap > 0) {
            if (!deps_f) deps_f = "";
            trim_in_place(deps_f);
            strncpy(deps_out, deps_f, deps_cap - 1);
            deps_out[deps_cap - 1] = '\0';
        }
        found = 1;
        break;
    }

    fclose(f);
    return found;
}

int pkgtree_init(void) {
    mkdir_p(".tree");
    mkdir_p(".tree/cache");
    mkdir_p(".tree/packages");
    if (!file_exists("tree.pkg")) {
        char *buf = fmt_alloc("# tree.pkg v2\nrepo=%s\nformat=2\n", repo_url);
        if (buf) {
            write_file("tree.pkg", buf);
            free(buf);
        }
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
    char *line = fmt_alloc("%s,%s", name, version);
    if (!line) return 0;
    if (append_line("tree.pkg", line) != 0) {
        free(line);
        fprintf(stderr, "[pkgtree][err] failed to append to tree.pkg\n");
        return 0;
    }
    free(line);
    printf("[pkgtree][ok] added dependency %s@%s\n", name, version);
    return 1;
}

int pkgtree_list(void) {
    FILE *f = fopen("tree.pkg", "r");
    if (!f) { printf("[pkgtree] no tree.pkg found\n"); return 0; }
    char buf[1024];
    int count = 0;
    load_repo_from_tree_pkg();
    printf("[pkgtree] repo: %s\n", repo_url);
    while (fgets(buf, sizeof(buf), f)) {
        char name[512];
        char ver[512];
        if (!parse_dep_line(buf, name, sizeof(name), ver, sizeof(ver))) continue;
        printf(" - %s@%s\n", name, ver);
        count++;
    }
    fclose(f);
    printf("[pkgtree] dependencies: %d\n", count);
    return count;
}

int pkgtree_install(void) {
    int installed_count = 0;
    mkdir_p(".tree");
    mkdir_p(".tree/cache");
    mkdir_p(".tree/packages");
    if (!file_exists("tree.pkg")) { fprintf(stderr, "[pkgtree][err] no tree.pkg found\n"); return 0; }
    load_repo_from_tree_pkg();

    FILE *f = fopen("tree.pkg","r");
    if (!f) return 0;
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        char name[512];
        char ver[512];
        char *pkgdir = NULL;
        char *url = NULL;
        char *srcpath = NULL;
        char *outpath = NULL;
        char desc[1024] = {0};
        char deps[1024] = {0};

        if (!parse_dep_line(line, name, sizeof(name), ver, sizeof(ver))) continue;
        pkgdir = fmt_alloc(".tree/packages/%s/%s", name, ver);
        outpath = fmt_alloc(".tree/packages/%s/%s/%s@%s.tree", name, ver, name, ver);

        if (pkgdir && outpath) {
            mkdir_p(pkgdir);

            if (repo_is_local(repo_url)) {
                srcpath = local_repo_package_path(name, ver);
                if (!local_manifest_find(name, ver, desc, sizeof(desc), deps, sizeof(deps))) {
                    fprintf(stderr, "[pkgtree][err] %s@%s not listed in %s/MANIFEST.tree\n", name, ver, repo_url);
                } else if (!srcpath || !file_exists(srcpath)) {
                    fprintf(stderr, "[pkgtree][err] local package missing: %s\n", srcpath ? srcpath : "(null)");
                } else if (copy_file(srcpath, outpath) != 0) {
                    fprintf(stderr, "[pkgtree][err] failed to copy %s -> %s\n", srcpath, outpath);
                } else {
                    printf("[pkgtree] installed %s@%s -> %s\n", name, ver, outpath);
                    if (desc[0]) printf("[pkgtree]   desc: %s\n", desc);
                    if (deps[0]) printf("[pkgtree]   deps: %s\n", deps);
                    installed_count++;
                }
            } else {
                url = fmt_alloc("%s/%s/%s/package.tree", repo_url, name, ver);
                if (url && try_fetch(url, outpath) == 0) {
                    printf("[pkgtree] fetched %s@%s -> %s\n", name, ver, outpath);
                    installed_count++;
                } else {
                    printf("[pkgtree] failed to fetch %s@%s\n", name, ver);
                }
            }
        } else {
            printf("[pkgtree] failed to install %s@%s\n", name, ver);
        }
        free(pkgdir);
        free(url);
        free(srcpath);
        free(outpath);
    }
    fclose(f);

    {
        char *lock = fmt_alloc("{locked=\"ok\", installed=%d}\n", installed_count);
        if (lock) {
            write_file("tree.lock", lock);
            free(lock);
        }
    }
    printf("[pkgtree][ok] install complete (%d package(s))\n", installed_count);
    return installed_count > 0;
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
    int found = 0;
    if (!pkg_name) return 0;
    load_repo_from_tree_pkg();

    if (!repo_is_local(repo_url)) {
        printf("[pkgtree] search currently supports local repo only (repo=%s)\n", repo_url);
        return 0;
    }

    {
        FILE *f;
        char line[2048];
        char *manifest = local_repo_manifest_path();
        if (!manifest) return 0;
        f = fopen(manifest, "r");
        free(manifest);
        if (!f) {
            fprintf(stderr, "[pkgtree][err] no local manifest found under %s\n", repo_url);
            return 0;
        }

        while (fgets(line, sizeof(line), f)) {
            char *name_f;
            char *ver_f;
            char *desc_f;
            trim_in_place(line);
            if (!*line || line[0] == '#') continue;
            name_f = strtok(line, "\t");
            ver_f = strtok(NULL, "\t");
            desc_f = strtok(NULL, "\t");
            if (!name_f || !ver_f) continue;
            trim_in_place(name_f);
            trim_in_place(ver_f);
            if (strcmp(name_f, pkg_name) != 0) continue;
            if (desc_f) trim_in_place(desc_f);
            printf("[pkgtree] found %s@%s as %s\n", name_f, ver_f, alias ? alias : pkg_name);
            if (desc_f && *desc_f) printf("[pkgtree]   desc: %s\n", desc_f);
            found++;
        }
        fclose(f);
    }

    if (found == 0) fprintf(stderr, "[pkgtree] package %s not found in %s/MANIFEST.tree\n", pkg_name, repo_url);
    return found;
}
