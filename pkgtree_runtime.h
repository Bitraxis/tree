// pkgtree_runtime.h
#ifndef PKGTREE_RUNTIME_H
#define PKGTREE_RUNTIME_H

#include <stdio.h>

#ifdef __GNUC__
#define PKGTREE_WEAK __attribute__((weak))
#else
#define PKGTREE_WEAK
#endif

#define PKGTREE_DEFAULT_REPO "repo"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PKGTREE_RUNTIME_IMPL
/* Declare prototypes and provide weak fallback implementations so the
   interpreter can be compiled and run without linking the full runtime. */
PKGTREE_WEAK int pkgtree_init(void);
PKGTREE_WEAK int pkgtree_set_repo(const char* url);
PKGTREE_WEAK int pkgtree_add(const char* name, const char* version);
PKGTREE_WEAK int pkgtree_install(void);
PKGTREE_WEAK int pkgtree_update(const char* name);
PKGTREE_WEAK int pkgtree_list(void);
PKGTREE_WEAK int pkgtree_pkg_search(const char* pkg_name, const char* alias);

/* Default (weak) implementations - they will be overridden if a strong
   implementation (pkgtree_runtime.c) is linked. */
PKGTREE_WEAK int pkgtree_init(void) {
    fprintf(stderr, "[pkgtree][stub] init called (no runtime)\n");
    return 0;
}
PKGTREE_WEAK int pkgtree_set_repo(const char* url) {
    fprintf(stderr, "[pkgtree][stub] set-repo: %s\n", url?url:"(null)");
    return 0;
}
PKGTREE_WEAK int pkgtree_add(const char* name, const char* version) {
    fprintf(stderr, "[pkgtree][stub] add: %s@%s\n", name?name:"", version?version:"");
    return 0;
}
PKGTREE_WEAK int pkgtree_install(void) {
    fprintf(stderr, "[pkgtree][stub] install called\n");
    return 0;
}
PKGTREE_WEAK int pkgtree_update(const char* name) {
    fprintf(stderr, "[pkgtree][stub] update: %s\n", name?name:"(null)");
    return 0;
}
PKGTREE_WEAK int pkgtree_list(void) {
    fprintf(stderr, "[pkgtree][stub] list called\n");
    return 0;
}
PKGTREE_WEAK int pkgtree_pkg_search(const char* pkg_name, const char* alias) {
    fprintf(stderr, "[pkgtree][stub] pkg search: %s (as %s)\n", pkg_name?pkg_name:"(null)", alias?alias:"(null)");
    return 0;
}

#else
/* When building the real runtime, pkgtree_runtime.c defines these. */
int pkgtree_init(void);
int pkgtree_set_repo(const char* url);
int pkgtree_add(const char* name, const char* version);
int pkgtree_install(void);
int pkgtree_update(const char* name);
int pkgtree_list(void);
int pkgtree_pkg_search(const char* pkg_name, const char* alias);
#endif

#ifdef __cplusplus
}
#endif

#endif /* PKGTREE_RUNTIME_H */
