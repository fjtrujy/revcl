"""Candidate-suite scope, compilation grouping and error handling."""
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("vuc_candidate_suite", ROOT / "vuc/scripts/match_candidates.py")
suite = importlib.util.module_from_spec(spec)
spec.loader.exec_module(suite)


class CandidateSuiteTests(unittest.TestCase):
    def test_string_table_add_has_exact_original_source_and_relocations(self):
        path = "vuc/config/string_table_add_oct14_probe.json"
        self.assertIn(path, json.loads(suite.EXACT_MANIFEST.read_text()))
        config = json.loads(suite.within_repo(path).read_text())
        self.assertEqual(config["address"], "0x08071880")
        self.assertEqual(config["size"], 263)
        self.assertEqual(config["source"], "vuc/src/MLGameStringTableAdd.cpp")
        self.assertEqual(len(config["relocations"]), 8)

    def test_baseline_manifest_has_unique_original_symbol_addresses(self):
        configs = json.loads(suite.DEFAULT_MANIFEST.read_text())
        records = [json.loads(suite.within_repo(p).read_text()) for p in configs]
        self.assertEqual(len(records), 212)
        # Local function spelling may repeat across original C++ units.
        self.assertEqual(len({(r["function"], r["address"]) for r in records}), len(records))

    def test_exact_regression_set_is_subset_of_diagnostics(self):
        all_configs = set(json.loads(suite.DEFAULT_MANIFEST.read_text()))
        exact_configs = json.loads(suite.EXACT_MANIFEST.read_text())
        self.assertEqual(len(exact_configs), 197)
        self.assertTrue(set(exact_configs).issubset(all_configs))
        for manifest in exact_configs:
            source = json.loads(suite.within_repo(manifest).read_text())["source"]
            self.assertTrue(source.startswith("vuc/src/"), source)
        for manifest in all_configs - set(exact_configs):
            source = json.loads(suite.within_repo(manifest).read_text())["source"]
            self.assertTrue(source.startswith("vuc/probes/"), source)

    def test_stmt_list_partial_is_tracked_but_never_exact(self):
        partial = "vuc/config/translate_stmt_list_partial_oct14_probe.json"
        selected = json.loads(suite.DEFAULT_MANIFEST.read_text())
        accepted = json.loads(suite.EXACT_MANIFEST.read_text())
        self.assertIn(partial, selected)
        self.assertNotIn(partial, accepted)
        manifest = json.loads(suite.within_repo(partial).read_text())
        self.assertEqual(manifest["address"], "0x0806ada0")
        self.assertEqual(manifest["size"], 119)
        self.assertEqual(manifest["flags"], ["-O2"])
        self.assertEqual(len(manifest["relocations"]), 3)

    def test_manifest_sources_and_configs_are_committed(self):
        """A passing local VM is insufficient if clean clones lack its inputs."""
        if not (ROOT / ".git").exists():
            self.skipTest("git working tree required for committed source audit")
        tracked = set(subprocess.check_output(
            ["git", "ls-files", "-z"], cwd=ROOT).decode().split("\0"))
        for filename in (suite.EXACT_MANIFEST, suite.DEFAULT_MANIFEST):
            for config_name in json.loads(filename.read_text()):
                with self.subTest(manifest=filename.name, config=config_name):
                    self.assertIn(config_name, tracked)
                    config = json.loads(suite.within_repo(config_name).read_text())
                    self.assertIn(config["source"], tracked)

    def test_string_singleton_getter_and_cleanup_are_both_exact(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        exploratory = set(json.loads(suite.DEFAULT_MANIFEST.read_text()))
        self.assertIn("vuc/config/ml_game_string_table_cleanup_probe.json", accepted)
        self.assertIn("vuc/config/ml_string_table_get_probe.json", accepted)
        self.assertIn("vuc/config/ml_string_table_get_probe.json", exploratory)

    def test_printer_unit_exactness_excludes_partial_ast_call(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        for filename in ("ugly_printer_unit_oct13_probe.json",
                         "iostream_global_printer_oct13_probe.json"):
            config_path = "vuc/config/" + filename
            self.assertIn(config_path, accepted)
            config = json.loads(suite.within_repo(config_path).read_text())
            self.assertEqual(config["source"], "vuc/src/Printer.cpp")
        self.assertNotIn("vuc/config/print_ast_fun_call_probe.json", accepted)

    def test_ast_matchstack_initializer_and_wrapper_share_real_source(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        for filename in ("ast_matchstack_init_oct13_probe.json",
                         "iostream_global_matchStack_probe.json"):
            path = "vuc/config/" + filename
            self.assertIn(path, accepted)
            self.assertEqual(json.loads(suite.within_repo(path).read_text())["source"],
                             "vuc/src/AST.cpp")
        helper = json.loads(suite.within_repo(
            "vuc/config/ast_matchstack_init_oct13_probe.json").read_text())
        self.assertEqual(helper["size"], 145)
        self.assertEqual(helper["relocation_objects"][".bss"]["matchStack"],
                         "0x0808abe4")
        self.assertEqual(helper["relocation_literals"][".rodata"][0]["size"], 14)

    def test_canonize_reduce_has_original_conversion_predecessors(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        targets = ("canonize2_context_exact_oct13.json",
                   "conv_val_float_exact_probe.json",
                   "conv_val_int_exact_probe.json",
                   "conv_val_uint_exact_probe.json")
        for filename in targets:
            config_path = "vuc/config/" + filename
            self.assertIn(config_path, accepted)
            config = json.loads(suite.within_repo(config_path).read_text())
            self.assertEqual(config["source"], "vuc/src/ConvVal.cpp")
            self.assertIn("-falign-jumps=1", config["flags"])
        self.assertNotIn("vuc/config/canonize2_reduce_probe.json", accepted)

    def test_value_constructor_unit_global_wrapper_is_exact(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        path = "vuc/config/value_global_genuine_context_oct13.json"
        self.assertIn(path, accepted)
        record = json.loads(suite.within_repo(path).read_text())
        self.assertEqual(record["address"], "0x0806f9d0")
        self.assertEqual(record["size"], 31)
        self.assertEqual(record["source"], "vuc/src/Value.cpp")
        self.assertEqual(record["relocations"][".text._Z41__static_initialization_and_destruction_0ii"],
                         "0x0806e6f0")
        for method in ("value_default_c1_probe.json", "value_default_c2_probe.json",
                       "value_dispatch_exact_probe.json"):
            self.assertIn("vuc/config/" + method, accepted)

    def test_value_multiply_uses_real_arithmetic_predecessors(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        manifest = "vuc/config/value_multiply_exact_oct13.json"
        self.assertIn(manifest, accepted)
        self.assertIn("vuc/config/value_arithmetic_minus_probe.json", accepted)
        config = json.loads(suite.within_repo(manifest).read_text())
        self.assertEqual(config["address"], "0x0806eb70")
        self.assertEqual(config["size"], 222)
        self.assertEqual(config["source"], "vuc/src/ValueArithmeticUnit.cpp")
        self.assertEqual(config["flags"],
                         ["-O2", "-frename-registers", "-falign-jumps=1"])
        self.assertEqual(config["relocations"],
                         {"_ZN5Value11TypeConvertERKS_S1_RS_S2_": "0x0806f7d0"})
        source = suite.within_repo(config["source"]).read_text()
        names = ("Value Value::operator+(", "Value Value::operator-(",
                 "Value Value::operator*(")
        self.assertEqual([source.index(name) for name in names],
                         sorted(source.index(name) for name in names))

    def test_value_type_convert_has_authentic_copy_and_shared_failure(self):
        config_name = "vuc/config/value_type_convert_exact_oct13.json"
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        self.assertIn(config_name, accepted)
        config = json.loads(suite.within_repo(config_name).read_text())
        self.assertEqual(config["address"], "0x0806f7d0")
        self.assertEqual(config["size"], 220)
        self.assertEqual(config["relocations"], {})
        self.assertEqual(config["flags"],
                         ["-O2", "-frename-registers", "-falign-jumps=1"])
        source = suite.within_repo(config["source"]).read_text()
        self.assertIn("left = first;", source)
        self.assertIn("right = second;", source)
        self.assertIn("switch (left.type)", source)
        self.assertIn("switch (right.type)", source)
        self.assertTrue(source.rstrip().endswith("return false;\n}"))

    def test_value_tostring_and_division_remain_experimental(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        exploratory = set(json.loads(suite.DEFAULT_MANIFEST.read_text()))
        for manifest, address, length in (
            ("vuc/config/value_to_string_oct13_probe.json", "0x0806f6f0", 221),
            ("vuc/config/value_division_partial_oct13.json", "0x0806ec50", 235),
        ):
            self.assertIn(manifest, exploratory)
            self.assertNotIn(manifest, accepted)
            config = json.loads(suite.within_repo(manifest).read_text())
            self.assertEqual(config["address"], address)
            self.assertEqual(config["size"], length)
            self.assertTrue(config["source"].startswith("vuc/probes/"))
        tostring = json.loads(suite.within_repo(
            "vuc/config/value_to_string_oct13_probe.json").read_text())
        ranges = tostring["relocation_literals"][".rodata"]
        self.assertEqual(sum(entry["size"] for entry in ranges), 82)
        self.assertEqual([entry["address"] for entry in ranges],
                         ["0x0807669c", "0x08072971", "0x08076690",
                          "0x08076699"])

    def test_value_forced_conversion_is_experimental_source_context(self):
        name = "vuc/config/value_type_convert_force_oct13_probe.json"
        exact = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        selected = set(json.loads(suite.DEFAULT_MANIFEST.read_text()))
        self.assertNotIn(name, exact)
        self.assertIn(name, selected)
        record = json.loads(suite.within_repo(name).read_text())
        self.assertEqual(record["address"], "0x0806f8b0")
        self.assertEqual(record["size"], 257)
        self.assertEqual(record["relocations"], {})
        self.assertEqual(record["source"], "vuc/probes/ValueTypeConvertForceUnit.cpp")
        source = suite.within_repo(record["source"]).read_text()
        self.assertLess(source.index("bool Value::TypeConvert("),
                        source.index("bool Value::TypeConvertForce("))

    def test_flex_nul_transition_preserves_signed_table_and_original_relocations(self):
        name = "vuc/config/yy_try_nul_trans_oct13_probe.json"
        exact = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        self.assertIn(name, exact)
        record = json.loads(suite.within_repo(name).read_text())
        self.assertEqual(record["address"], "0x080614c0")
        self.assertEqual(record["size"], 161)
        self.assertEqual(record["source"], "vuc/src/YyTryNulTrans.cpp")
        self.assertEqual(record["flags"], ["-O2", "-frename-registers"])
        self.assertEqual(record["relocations"], {
            "yy_accept": "0x08074f40", "yy_base": "0x08075540",
            "yy_def": "0x08075660", "yy_chk": "0x080759a0",
            "yy_nxt": "0x08075780", "yy_meta": "0x08075460",
        })
        source = suite.within_repo(record["source"]).read_text()
        self.assertIn("extern const short yy_base[];", source)
        self.assertNotIn("extern const unsigned short yy_base[];", source)
        self.assertIn("while (yy_chk[yy_base[yy_current_state] + yy_c]", source)

    def test_flex_previous_state_uses_signed_tables_and_original_load_order(self):
        name = "vuc/config/yy_get_previous_state_oct13_probe.json"
        exact = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        self.assertIn(name, exact)
        record = json.loads(suite.within_repo(name).read_text())
        self.assertEqual(record["address"], "0x080613f0")
        self.assertEqual(record["size"], 205)
        self.assertEqual(record["source"], "vuc/src/YyGetPreviousState.cpp")
        self.assertEqual(record["flags"], ["-O2", "-frename-registers"])
        self.assertEqual(record["relocations"], {
            "yy_accept": "0x08074f40", "yy_ec": "0x08075060",
            "yy_base": "0x08075540", "yy_def": "0x08075660",
            "yy_chk": "0x080759a0", "yy_nxt": "0x08075780",
            "yy_meta": "0x08075460",
        })
        source = suite.within_repo(record["source"]).read_text()
        self.assertIn("extern const short yy_base[];", source)
        self.assertLess(source.index("unsigned char yy_ch = *yy_cp;"),
                        source.index("unsigned char yy_c = 1;"))

    def test_flex_yyinput_exact_original_eof_fallthrough(self):
        name = "vuc/config/yyinput_reconstruct_oct13_probe.json"
        exact = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        self.assertIn(name, exact)
        config = json.loads(suite.within_repo(name).read_text())
        self.assertEqual(config["address"], "0x08061620")
        self.assertEqual(config["size"], 170)
        self.assertEqual(config["flags"], ["-O2", "-frename-registers"])
        self.assertEqual(config["source"], "vuc/src/YyInput.cpp")
        self.assertEqual(config["relocations"], {
            "_ZN11yyFlexLexer18yy_get_next_bufferEv": "0x080611c0",
        })
        src = suite.within_repo(config["source"]).read_text()
        self.assertIn("case 2:\n                yyrestart(yyin);\n            case 1:", src)
        self.assertIn("int offset = yy_c_buf_p - yytext_ptr;", src)

    def test_flex_yyunput_exact_buffer_shift_and_error_literal(self):
        name = "vuc/config/yyunput_reconstruct_oct13_probe.json"
        exact = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        self.assertIn(name, exact)
        config = json.loads(suite.within_repo(name).read_text())
        self.assertEqual(config["address"], "0x08061570")
        self.assertEqual(config["size"], 167)
        self.assertEqual(config["flags"], ["-O2", "-frename-registers"])
        self.assertEqual(config["source"], "vuc/src/YyUnput.cpp")
        self.assertEqual(config["relocations"], {})
        self.assertEqual(config["relocation_literals"][".rodata"], [
            {"offset": 0, "size": 32, "address": "0x08074dc0"},
        ])
        src = suite.within_repo(config["source"]).read_text()
        self.assertIn("*--dest = *--source;", src)
        self.assertIn('LexerError("flex scanner push-back overflow")', src)

    def test_flex_next_buffer_remains_a_relocation_checked_partial(self):
        manifest = "vuc/config/yy_get_next_buffer_oct13_probe.json"
        exact = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        exploratory = set(json.loads(suite.DEFAULT_MANIFEST.read_text()))
        self.assertNotIn(manifest, exact)
        self.assertIn(manifest, exploratory)
        config = json.loads(suite.within_repo(manifest).read_text())
        self.assertEqual(config["address"], "0x080611c0")
        self.assertEqual(config["size"], 550)
        self.assertEqual(config["source"], "vuc/probes/yy_get_next_buffer_partial_oct13.cpp")
        self.assertEqual(config["flags"],
                         ["-O2", "-frename-registers", "-falign-jumps=1"])
        self.assertEqual(config["relocations"], {"realloc": "0x0804988c"})
        ranges = config["relocation_literals"][".rodata"]
        self.assertEqual(sum(item["size"] for item in ranges), 152)
        self.assertEqual([item["address"] for item in ranges if item["size"] > 20],
                         ["0x08075ba6", "0x08074d40", "0x08074d80"])
        source = suite.within_repo(config["source"]).read_text()
        self.assertIn("while (num_to_read <= 0)", source)
        self.assertIn("std::realloc(old_buf, b->yy_buf_size + 2)", source)

    def test_align_string_promotes_only_authenticated_minimal_source(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        exploratory = set(json.loads(suite.DEFAULT_MANIFEST.read_text()))
        new = "vuc/config/align_string_exact_oct13.json"
        self.assertIn(new, accepted)
        self.assertIn(new, exploratory)
        self.assertNotIn("vuc/config/align_string_probe.json", accepted)
        self.assertNotIn("vuc/config/align_string_probe.json", exploratory)
        config = json.loads(suite.within_repo(new).read_text())
        self.assertEqual(config["address"], "0x08049b80")
        self.assertEqual(config["size"], 145)
        self.assertEqual(config["source"], "vuc/src/AlignString.cpp")
        self.assertEqual(config["flags"],
                         ["-O2", "-frename-registers", "-falign-jumps=4"])
        self.assertEqual(config["relocations"], {
            "strcpy": "0x08049a0c", "strchr": "0x0804962c",
            "strcat": "0x0804989c",
        })
        self.assertEqual(config["relocation_objects"][".bss"], {
            "_ZZ11AlignStringPKciE9formatted": "0x0808aae0",
        })
        self.assertNotIn("relocation_literals", config)
        source = suite.within_repo(config["source"]).read_text()
        self.assertIn("static char formatted[256];", source)
        self.assertIn("char *space = strchr(formatted, ' ');", source)
        self.assertNotIn("AsmOutput_WritePrologue", source)

    def test_currfile_wrapper_has_real_diagnostic_source_context(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        config_name = "vuc/config/iostream_currfile_real_context_oct13.json"
        self.assertIn(config_name, accepted)
        config = json.loads(suite.within_repo(config_name).read_text())
        self.assertEqual(config["source"], "vuc/src/VucCompilerUnit.cpp")
        self.assertEqual(config["address"], "0x08071580")
        self.assertEqual(config["size"], 31)
        self.assertEqual(config["relocations"][".text._Z41__static_initialization_and_destruction_0ii"],
                         "0x0806f9f0")

    def test_symboltable_startup_is_exact_without_promoting_enter(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        wrapper = "vuc/config/iostream_global_symboltable_oct13_probe.json"
        self.assertIn(wrapper, accepted)
        info = json.loads(suite.within_repo(wrapper).read_text())
        self.assertEqual(info["address"], "0x0806a530")
        self.assertEqual(info["size"], 31)
        self.assertEqual(info["source"], "vuc/src/SymbolTableUnit.cpp")
        self.assertEqual(info["relocations"][".text._Z41__static_initialization_and_destruction_0ii"],
                         "0x0806a2e0")
        self.assertNotIn("vuc/config/symboltable_enter_oct13_probe.json", accepted)
        for neighbor in ("symboltable_find_probe.json", "symbol_begin_scope_probe.json"):
            self.assertIn("vuc/config/" + neighbor, accepted)

    def test_listfile_global_wrapper_has_recovered_source_neighbors(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        wrapper = "vuc/config/iostream_listfile_real_oct13_probe.json"
        self.assertIn(wrapper, accepted)
        info = json.loads(suite.within_repo(wrapper).read_text())
        self.assertEqual(info["address"], "0x0805b020")
        self.assertEqual(info["size"], 31)
        self.assertEqual(info["source"], "vuc/src/ListFileUnit.cpp")
        self.assertEqual(info["relocations"][".text._Z41__static_initialization_and_destruction_0ii"],
                         "0x0805a5c0")
        for neighbor in ("list_file_lines_probe.json", "listfile_print_probe.json",
                         "listfile_printline_probe.json"):
            self.assertIn("vuc/config/" + neighbor, accepted)

    def test_partial_source_contexts_are_not_exactly_accepted(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        exploratory = set(json.loads(suite.DEFAULT_MANIFEST.read_text()))
        for name in ("symboltable_enter_oct13_probe.json",
                     "instruction_arena_cleanup_unit_oct14_probe.json"):
            path = "vuc/config/" + name
            self.assertIn(path, exploratory)
            self.assertNotIn(path, accepted)
            self.assertTrue(json.loads(suite.within_repo(path).read_text())["source"].startswith(
                "vuc/probes/"))

        historical = "vuc/config/yy_load_full_unit_oct14_probe.json"
        self.assertNotIn(historical, exploratory)
        self.assertNotIn(historical, accepted)
        self.assertTrue(suite.within_repo(historical).is_file())

    def test_semant_global_wrapper_preserves_exact_call_and_entrypoint(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        manifest = "vuc/config/iostream_semant_real_oct13_probe.json"
        self.assertIn(manifest, accepted)
        config = json.loads(suite.within_repo(manifest).read_text())
        self.assertEqual(config["source"], "vuc/src/SemantUnit.cpp")
        self.assertEqual(config["address"], "0x0806a0c0")
        self.assertEqual(config["size"], 31)
        self.assertEqual(config["relocations"][".text._Z41__static_initialization_and_destruction_0ii"],
                         "0x080619d0")
        self.assertIn("vuc/config/check_ast_fun_call_probe.json", accepted)
        self.assertIn("vuc/config/semant_check_exact_probe.json", accepted)

    def test_translate_type_startup_wrapper_is_original_exact(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        manifest = "vuc/config/iostream_translate_real_oct13_probe.json"
        self.assertIn(manifest, accepted)
        config = json.loads(suite.within_repo(manifest).read_text())
        self.assertEqual(config["source"], "vuc/src/TranslateTypeUnit.cpp")
        self.assertEqual(config["address"], "0x0806e050")
        self.assertEqual(config["size"], 31)
        self.assertEqual(config["relocations"][".text._Z41__static_initialization_and_destruction_0ii"],
                         "0x0806a960")

    def test_rebased_source_unit_startups_are_exact_but_bodies_not_promoted(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        original_wrappers = (
            ("inline_asm_startup_context_oct13_probe.json", "0x08051270",
             "0x08050ce0", "InlineAsmUnit.cpp"),
            ("ir2asm_broadcast_startup_oct13_probe.json", "0x080561a0",
             "0x08051ae0", "IR2AsmBroadcastUnit.cpp"),
            ("ir2asm_binexpr_startup_oct13_probe.json", "0x0805a4d0",
             "0x080561c0", "IR2AsmBinExprUnit.cpp"),
            ("script_debug_real_parser_oct13_probe.json", "0x08060790",
             "0x0805cce0", "ScriptParserUnit.cpp"),
            ("lexer_yylex_startup_oct13_probe.json", "0x08061980",
             "0x080607c0", "ScriptScannerUnit.cpp"),
        )
        for filename, address, initializer, source in original_wrappers:
            with self.subTest(filename=filename):
                manifest = "vuc/config/" + filename
                self.assertIn(manifest, accepted)
                config = json.loads(suite.within_repo(manifest).read_text())
                self.assertEqual(config["address"], address)
                self.assertEqual(config["size"], 31)
                self.assertEqual(config["source"], "vuc/src/" + source)
                self.assertEqual(config["relocations"][
                    ".text._Z41__static_initialization_and_destruction_0ii"],
                    initializer)
        scanner = json.loads(suite.within_repo(
            "vuc/config/lexer_yylex_startup_oct13_probe.json").read_text())
        self.assertNotIn("-frename-registers", scanner["flags"])
        self.assertNotIn("vuc/config/iostream_global_script_debug_probe.json", accepted)

    def test_type_is_assignable_exact_uses_original_branch_shape(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        manifest = "vuc/config/type_is_assignable_probe.json"
        self.assertIn(manifest, accepted)
        config = json.loads(suite.within_repo(manifest).read_text())
        self.assertEqual(config["function"], "_ZNK4Type12IsAssignableERKS_b")
        self.assertEqual(config["address"], "0x0806a1c0")
        self.assertEqual(config["size"], 151)
        self.assertEqual(config["source"], "vuc/src/TypeAssignable.cpp")
        self.assertIn("-falign-jumps=4", config["flags"])
        self.assertEqual(config["relocations"], {})
        source = suite.within_repo(config["source"]).read_text()
        self.assertIn("(other.flags & flags) == flags", source)
        self.assertIn("return false;", source)

    def test_lexer_buffer_load_uses_original_store_order(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        path = "vuc/config/yy_load_buffer_probe.json"
        self.assertIn(path, accepted)
        config = json.loads(suite.within_repo(path).read_text())
        self.assertEqual(config["address"], "0x08061780")
        self.assertEqual(config["size"], 39)
        self.assertEqual(config["source"], "vuc/src/FlexLexer.cpp")
        self.assertEqual(config["relocations"], {})
        source = suite.within_repo(config["source"]).read_text()
        assignments = ["yy_n_chars_ = current_->yy_n_chars;",
                       "yy_c_buf_p_ = current_->yy_buf_pos;",
                       "yytext_ptr_ = yy_c_buf_p_;",
                       "input_ = current_->file;",
                       "yy_hold_char_ = *yy_c_buf_p_;"]
        self.assertEqual(sorted(source.index(a) for a in assignments),
                         [source.index(a) for a in assignments])

    def test_string_table_get_real_destructor_relocation(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        path = "vuc/config/ml_string_table_get_probe.json"
        self.assertIn(path, accepted)
        config = json.loads(suite.within_repo(path).read_text())
        self.assertEqual(config["address"], "0x08071810")
        self.assertEqual(config["size"], 106)
        self.assertEqual(config["source"], "vuc/src/MLGameStringTableUnit.cpp")
        self.assertIn("-ffunction-sections", config["flags"])
        self.assertEqual(config["relocations"][".text.__tcf_0"], "0x08071e00")
        self.assertNotIn(".text", config["relocations"])

    def test_instruction_list_variadic_add_and_addb_are_exact(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        checks = (
            ("instruction_add_format_probe.json", "0x08051570",
             "_ZN15InstructionList3AddEPKcz"),
            ("instruction_addb_format_exact_oct13_probe.json", "0x08051610",
             "_ZN15InstructionList4AddBEPKcz"),
        )
        for filename, address, function in checks:
            with self.subTest(filename=filename):
                path = "vuc/config/" + filename
                self.assertIn(path, accepted)
                config = json.loads(suite.within_repo(path).read_text())
                self.assertEqual(config["source"], "vuc/src/InstructionList.cpp")
                self.assertEqual(config["function"], function)
                self.assertEqual(config["address"], address)
                self.assertEqual(config["size"], 146)
                self.assertEqual(config["relocations"]["vsprintf"], "0x0804961c")
                self.assertEqual(config["relocations"]["_ZN11InstructionnwEj"],
                                 "0x08051910")
                self.assertEqual(config["relocations"]["_Unwind_Resume"],
                                 "0x0804998c")
        self.assertEqual(len(config["relocations"]), 7)

        line_comment = "vuc/config/instruction_add_line_comment_context_oct13_probe.json"
        experimental = set(json.loads(suite.DEFAULT_MANIFEST.read_text()))
        self.assertIn(line_comment, experimental)
        self.assertIn(line_comment, accepted)
        near_config = json.loads(suite.within_repo(line_comment).read_text())
        self.assertEqual(near_config["source"],
                         "vuc/src/InstructionListLineCommentUnit.cpp")
        self.assertEqual(near_config["flags"],
                         ["-O2", "-frename-registers", "-falign-jumps=8"])
        self.assertEqual(near_config["size"], 162)
        self.assertEqual(sum(r["size"] for r in
                             near_config["relocation_literals"][".rodata"]), 22)

        set_probe = "vuc/config/ml_temp_set_probe.json"
        self.assertIn(set_probe, experimental)
        self.assertNotIn(set_probe, accepted)
        set_config = json.loads(suite.within_repo(set_probe).read_text())
        self.assertEqual(set_config["address"], "0x08071d00")
        self.assertEqual(set_config["size"], 117)
        self.assertEqual(set_config["source"],
                         "vuc/probes/ml_temp_set_return.cpp")
        self.assertEqual(set_config["flags"], ["-O2"])
        self.assertEqual(len(set_config["relocations"]), 2)

    def test_asmoutput_tab_constants_is_exact_in_real_source_order(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        config_name = "vuc/config/asm_output_tab_constants_probe.json"
        self.assertIn(config_name, accepted)
        config = json.loads(suite.within_repo(config_name).read_text())
        self.assertEqual(config["function"], "_Z22AsmOutput_TabConstantsR15InstructionList")
        self.assertEqual(config["address"], "0x08049df0")
        self.assertEqual(config["size"], 127)
        self.assertEqual(config["source"], "vuc/src/AsmOutputUnit.cpp")
        self.assertIn("-frename-registers", config["flags"])
        self.assertEqual(config["relocations"]["strstr"], "0x0804981c")
        self.assertEqual(config["relocations"]["constantsTabStop"], "0x0808aac4")
        literals = config["relocation_literals"][".rodata"]
        self.assertEqual([l["size"] for l in literals], [92, 19, 3, 8, 5])
        self.assertEqual(sum(l["size"] for l in literals), 127)
        self.assertEqual([l["address"] for l in literals],
                         ["0x080720a0", "0x080721c4", "0x080721d7",
                          "0x080721f0", "0x080721fb"])
        source = suite.within_repo(config["source"]).read_text()
        names = ("void AsmOutput_WritePrologue", "void AsmOutput_WriteEpilogue",
                 "char *AlignString", "char *AlignConstant",
                 "void AsmOutput_TabConstants")
        self.assertEqual(sorted(source.index(name) for name in names),
                         [source.index(name) for name in names])

    def test_align_string_historical_partial_remains_documented(self):
        selected = set(json.loads(suite.DEFAULT_MANIFEST.read_text()))
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        path = "vuc/config/align_string_probe.json"
        self.assertNotIn(path, selected)
        self.assertNotIn(path, accepted)
        config = json.loads(suite.within_repo(path).read_text())
        self.assertEqual(config["address"], "0x08049b80")
        self.assertEqual(config["size"], 145)
        self.assertEqual(config["source"], "vuc/probes/align_string_real_unit_oct13.cpp")
        self.assertEqual(config["flags"],
                         ["-O2", "-frename-registers", "-falign-jumps=4"])
        self.assertIn('#include "../src/AsmOutputUnit.cpp"',
                      suite.within_repo(config["source"]).read_text())
        self.assertEqual(config["relocation_objects"][".bss"]
                         ["_ZZ11AlignStringPKciE9formatted"], "0x0808aae0")
        self.assertEqual(sum(x["size"] for x in config["relocation_literals"]
                             [".rodata"]), 127)

    def test_align_constant_exact_shares_real_asm_source_unit(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        path = "vuc/config/align_constant_exact_probe.json"
        self.assertIn(path, accepted)
        config = json.loads(suite.within_repo(path).read_text())
        self.assertEqual(config["address"], "0x08049c20")
        self.assertEqual(config["size"], 69)
        self.assertEqual(config["source"], "vuc/src/AsmOutputUnit.cpp")
        self.assertEqual(config["flags"],
                         ["-O2", "-frename-registers", "-falign-jumps=4"])
        self.assertNotIn(".rodata", config["relocations"])
        self.assertEqual(len(config["relocations"]), 3)
        self.assertEqual(sum(x["size"] for x in config["relocation_literals"]
                             [".rodata"]), 127)

    def test_new_partial_controls_remain_outside_exact_manifest(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        selected = set(json.loads(suite.DEFAULT_MANIFEST.read_text()))
        cases = (
            ("script_error_probe.json", 161, "0x0806fa80"),
            ("yy_create_buffer_probe.json", 139, "0x080617b0"),
            ("ir_accumulatorable_probe.json", 77, "0x0805a570"),
        )
        for name, size, address in cases:
            with self.subTest(config=name):
                path = "vuc/config/" + name
                self.assertIn(path, selected)
                self.assertNotIn(path, accepted)
                config = json.loads(suite.within_repo(path).read_text())
                self.assertEqual(config["size"], size)
                self.assertEqual(config["address"], address)
                self.assertTrue(config["source"].startswith("vuc/probes/"))
                if name == "ir_accumulatorable_probe.json":
                    self.assertEqual(config["source"],
                                     "vuc/probes/ir_accumulatorable_joint_oct14.cpp")
                    self.assertEqual(config["flags"],
                                     ["-O2", "-frename-registers", "-falign-jumps=8"])

    def test_export_module_lifecycle_is_exact_but_reserve_is_not(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        for name in ("scope_vector_teardown_probe.json",
                     "export_module_scope_wrapper_probe.json",
                     "export_module_static_initializer_probe.json"):
            self.assertIn("vuc/config/" + name, accepted)
        self.assertNotIn("vuc/config/scope_vector_reserve_probe.json", accepted)

    def test_original_tu_flag_hypothesis_is_explicit(self):
        with tempfile.TemporaryDirectory() as out:
            with patch.object(suite.subprocess, "run",
                              return_value=subprocess.CompletedProcess([], 0, "", "")) as run_mock:
                with patch.object(suite.matcher, "run", return_value={
                    "function": "c_str", "exact": True, "exact_bytes": 13,
                    "length": 13, "verified_literal_bytes": 0}):
                    result = suite.run("compiler", out,
                                       ["vuc/config/ml_game_string_cstr_probe.json"])
            command = run_mock.call_args.args[0]
            self.assertIn("-frename-registers", command)
            self.assertEqual(result["results"][0]["candidate_flags"],
                             ["-O2", "-frename-registers"])

    def test_cli_fails_for_any_partial_function(self):
        result = {"all_candidates_exact": False, "acceptance": "FAIL",
                  "fully_exact_functions": 1, "functions_checked": 2,
                  "matching_code_bytes": 9, "checked_code_bytes": 18}
        with tempfile.TemporaryDirectory() as out:
            with patch("sys.argv", ["match_candidates.py", "--compiler", "mock-gcc",
                                    "--out", out]):
                with patch.object(suite, "run", return_value=result):
                    self.assertEqual(suite.main(), 1)

    def test_rejects_path_escape(self):
        with self.assertRaisesRegex(ValueError, "escapes repository"):
            suite.within_repo("../outside")

    def test_aggregates_only_checked_functions(self):
        configs = ["vuc/config/list_file_lines_probe.json",
                   "vuc/config/ml_game_string_cstr_probe.json"]
        results = [
            {"function": "ListFile", "exact": True, "exact_bytes": 18,
             "length": 18, "verified_literal_bytes": 0},
            {"function": "MLGameString", "exact": False, "exact_bytes": 10,
             "length": 13, "verified_literal_bytes": 0},
        ]
        with tempfile.TemporaryDirectory() as out:
            with patch.object(suite.subprocess, "run",
                              return_value=subprocess.CompletedProcess([], 0, "", "")) as compile_mock:
                with patch.object(suite.matcher, "run", side_effect=results):
                    summary = suite.run("compiler", out, configs)
            self.assertEqual(compile_mock.call_count, 2)
            self.assertEqual(summary["functions_checked"], 2)
            self.assertEqual(summary["fully_exact_functions"], 1)
            self.assertEqual(summary["matching_code_bytes"], 28)
            self.assertEqual(summary["checked_code_bytes"], 31)
            self.assertIsNone(summary["whole_binary_score"])
            self.assertFalse(summary["all_candidates_exact"])
            self.assertEqual(summary["acceptance"], "FAIL")
            self.assertTrue((Path(out) / "candidate_summary.json").is_file())

    def test_exact_candidates_can_pass_only_without_partial_matches(self):
        with tempfile.TemporaryDirectory() as out:
            with patch.object(suite.subprocess, "run",
                              return_value=subprocess.CompletedProcess([], 0, "", "")):
                with patch.object(suite.matcher, "run", return_value={
                    "function": "ListFile", "exact": True, "exact_bytes": 18,
                    "length": 18, "verified_literal_bytes": 0}):
                    summary = suite.run("compiler", out,
                                        ["vuc/config/list_file_lines_probe.json"])
            self.assertTrue(summary["all_candidates_exact"])
            self.assertEqual(summary["acceptance"], "PASS")
            self.assertIsNone(summary["whole_binary_score"])

    def test_compiler_failure_is_not_a_match(self):
        with tempfile.TemporaryDirectory() as out:
            with patch.object(suite.subprocess, "run",
                              return_value=subprocess.CompletedProcess([], 1, "", "compile error")):
                with self.assertRaisesRegex(RuntimeError, "compile error"):
                    suite.run("compiler", out, ["vuc/config/list_file_lines_probe.json"])
            self.assertFalse((Path(out) / "candidate_summary.json").exists())

    def test_duplicate_function_rejected(self):
        config = "vuc/config/list_file_lines_probe.json"
        with tempfile.TemporaryDirectory() as out:
            with patch.object(suite.subprocess, "run",
                              return_value=subprocess.CompletedProcess([], 0, "", "")):
                with patch.object(suite.matcher, "run",
                                  return_value={"exact": True, "exact_bytes": 18,
                                                "length": 18, "verified_literal_bytes": 0}):
                    with self.assertRaisesRegex(ValueError, "duplicate function"):
                        suite.run("compiler", out, [config, config])

    def test_duplicate_local_name_with_distinct_original_addresses_is_allowed(self):
        paths = ["vuc/config/iostream_cleanup_0804a6f0_probe.json",
                 "vuc/config/iostream_cleanup_0804a7c0_probe.json"]
        result = {"function": "__tcf_0", "exact": True, "exact_bytes": 16,
                  "length": 16, "verified_literal_bytes": 0}
        with tempfile.TemporaryDirectory() as out:
            with patch.object(suite.subprocess, "run",
                              return_value=subprocess.CompletedProcess([], 0, "", "")) as comp:
                with patch.object(suite.matcher, "run", return_value=result):
                    summary = suite.run("compiler", out, paths)
        self.assertEqual(comp.call_count, 1)
        self.assertEqual(summary["functions_checked"], 2)
        self.assertEqual(summary["checked_code_bytes"], 32)
        self.assertTrue(summary["all_candidates_exact"])


if __name__ == "__main__":
    unittest.main()
