/* radare2 - LGPL - Copyright 2015-2022 pancake */

#include "r_lib.h"
#include "r_core.h"
#include "r_lang.h"

#include "../js_require.c"

const char *const js_entrypoint_qjs = "Gmain(requirejs,global,{r2:r2})";
#if 0
	"requirejs (['asan'], function (foo) {\n"\
	"  foo.main(r2);\n" \
	"});\n"
;
#endif
// console.log("foo", foo.main);

static bool lang_tsc_file(RLangSession *s, const char *file) {
	if (!r_str_endswith (file, ".ts")) {
		R_LOG_WARN ("expecting .ts extension");
		return false;
	}
	if (!r_file_exists (file)) {
		R_LOG_WARN ("file does tno exist");
		return false;
	}
	char *js_ofile = strdup (file);
	js_ofile = r_str_replace (js_ofile, ".ts", ".js", 0);
	char *qjs_ofile = strdup (file);
	qjs_ofile = r_str_replace (qjs_ofile, ".ts", ".qjs", 0);
	int rc = 0;
	/// check of ofile exists and its newer than file
	if (!r_file_is_newer (qjs_ofile, file)) {
		rc = r_sys_cmdf ("tsc --outFile %s --lib es2020,dom --moduleResolution node --module amd %s", js_ofile, file);
		if (rc == 0) {
			char *js_ifile = r_file_slurp (js_ofile, NULL);
			RStrBuf *sb = r_strbuf_new ("var Gmain;");
			r_strbuf_append (sb, js_require_qjs);
			js_ifile = r_str_replace (js_ifile,
				"function (require, exports, r2papi_1) {",
				"Gmain=function (require, exports, r2papi_1) {", 1);
			r_strbuf_append (sb, js_ifile);
			r_strbuf_append (sb, js_entrypoint_qjs);
			char *s = r_strbuf_drain (sb);
			r_file_dump (qjs_ofile, (const ut8*)s, -1, 0);
			free (s);
			r_file_rm (js_ofile);
		}
	} else { eprintf ("no need to compile\n"); }
	if (rc == 0) {
		r_lang_use (s->lang, "qjs");
		rc = r_lang_run_file (s->lang, qjs_ofile)? 0: -1;
	}
	free (js_ofile);
	free (qjs_ofile);
	return rc;
}

static bool lang_tsc_run(RLangSession *s, const char *code, int len) {
	char *ts_ofile = r_str_newf (".tmp.ts");
	bool rv = r_file_dump (ts_ofile, (const ut8 *)code, len, 0);
	if (rv) {
		rv = lang_tsc_file (s, ts_ofile);
	}
	free (ts_ofile);
	return rv;
}

static RLangPlugin r_lang_plugin_tsc = {
	.name = "tsc",
	.ext = "ts",
	.author = "pancake",
	.license = "LGPL",
	.desc = "Use #!tsc script.ts",
	.run = lang_tsc_run,
	.run_file = (void*)lang_tsc_file,
};
