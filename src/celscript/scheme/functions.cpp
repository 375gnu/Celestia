#include <string_view>
#include <celestia/celestiacore.h>
#include "functions.h"

namespace
{

CelestiaCore*
getAppCore(sexp ctx)
{
    sexp pointer = sexp_env_ref(ctx, sexp_context_env(ctx), sexp_intern(ctx, "*app-core*", -1), SEXP_NULL);
    return static_cast<CelestiaCore*>(sexp_cpointer_value(pointer));
}

sexp
celestia_print(sexp ctx, sexp self, sexp_sint_t n, sexp arg1)
{
    if (!sexp_stringp(arg1))
        return sexp_type_exception(ctx, self, SEXP_STRING, arg1);

    int horig = 1;
    int vorig = -1;
    int hoff = -1;
    int voff = 0;
    double duration = 1.5;

    std::string_view s(sexp_string_data(arg1), sexp_string_size(arg1));
    getAppCore(ctx)->showText(s, horig, vorig, hoff, voff, duration);

    return SEXP_VOID;
}
}

void
SetupSchemeFunctions(sexp ctx, sexp env)
{
    sexp_define_foreign(ctx, env, "celestia-print", 1, celestia_print);
}
