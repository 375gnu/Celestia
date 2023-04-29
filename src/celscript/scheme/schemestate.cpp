#include <iostream>
#include <sstream>
#include <chibi/eval.h>
#include <celestia/celestiacore.h>
#include "schemestate.h"

namespace celestia::scripts
{

SchemeState::SchemeState(CelestiaCore *appCore) :
    m_appCore(appCore)
{
    sexp_scheme_init();
    m_ctx = sexp_make_eval_context(nullptr, nullptr, nullptr, 0, 0);
    sexp_load_standard_env(m_ctx, nullptr, SEXP_SEVEN);
    sexp_load_standard_ports(m_ctx, nullptr, /*stdin*/nullptr, /*stdout*/nullptr, /*stderr*/nullptr, 1);

    sexp core = sexp_make_cpointer(m_ctx, SEXP_CPOINTER, m_appCore, nullptr, 0);
    sexp symbol = sexp_intern(m_ctx, "*app-core*", -1);
    sexp env = sexp_context_env(m_ctx);

    sexp_env_define(m_ctx, env, symbol, core);
}

SchemeState::~SchemeState()
{
    sexp_destroy_context(m_ctx);
}

bool
SchemeState::loadScript(const fs::path &path)
{
    // declare and preserve local variables
    sexp_gc_var1(obj1);
    sexp_gc_preserve1(m_ctx, obj1);

    obj1 = sexp_c_string(m_ctx, path.string().s_str(), -1);
    sexp_load(m_ctx, obj1, nullptr);

    // release the local variables
    sexp_gc_release1(m_ctx);
}

bool
SchemeState::loadScript(const std::string &str)
{
    sexp_eval_string(m_ctx, str.c_str(), -1, nullptr);
}

} // namespace celestia::scripts
