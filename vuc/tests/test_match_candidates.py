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
    def test_asm_variable_vector_insert_is_generated_by_real_inline_asm_unit(self):
        """Historical vector code is emitted from a real AST binding method."""
        path = "vuc/config/asm_variable_vector_insert_exact_oct15i.json"
        self.assertIn(path, json.loads(suite.EXACT_MANIFEST.read_text()))
        config = json.loads(suite.within_repo(path).read_text())
        self.assertEqual(config["address"], "0x08051290")
        self.assertEqual(config["size"], 444)
        self.assertEqual(config["source"], "vuc/src/InlineAsmUnit.cpp")
        self.assertEqual(config["flags"], ["-O2", "-frename-registers"])
        self.assertEqual(len(config["relocations"]), 7)
        self.assertIn("variables.push_back(variable)",
                      suite.within_repo(config["source"]).read_text())

    def test_scope_vector_insert_partial_remains_experimental(self):
        """A 733/746 candidate is original-length, not exact acceptance."""
        path = "vuc/config/scope_vector_insert_partial_oct15i.json"
        self.assertIn(path, json.loads(suite.DEFAULT_MANIFEST.read_text()))
        self.assertNotIn(path, json.loads(suite.EXACT_MANIFEST.read_text()))
        config = json.loads(suite.within_repo(path).read_text())
        self.assertEqual(config["address"], "0x0806a550")
        self.assertEqual(config["size"], 746)
        self.assertEqual(config["source"],
                         "vuc/probes/ScopeVectorInsertionOriginalUnitOct15i.cpp")
        self.assertEqual(len(config["relocations"]), 9)

    def test_instruction_vector_insert_is_recovered_from_real_instruction_unit(self):
        """The STL helper is instantiated by authentic InstructionList methods."""
        path = "vuc/config/instruction_vector_insert_exact_oct15i.json"
        self.assertIn(path, json.loads(suite.EXACT_MANIFEST.read_text()))
        config = json.loads(suite.within_repo(path).read_text())
        self.assertEqual(config["address"], "0x080519d0")
        self.assertEqual(config["size"], 265)
        self.assertEqual(config["flags"], ["-O2", "-frename-registers", "-falign-jumps=1"])
        self.assertEqual(config["source"], "vuc/src/InstructionListLineCommentUnit.cpp")
        self.assertEqual(config["relocations"]["memmove"], "0x0804978c")
        self.assertEqual(len(config["relocations"]), 3)
        source = suite.within_repo(config["source"]).read_text()
        self.assertIn("instructions_.push_back(instruction)", source)
        self.assertIn("std::vector<Instruction *>", source)

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
        self.assertEqual(len(records), 240)
        # Local function spelling may repeat across original C++ units.
        self.assertEqual(len({(r["function"], r["address"]) for r in records}), len(records))

    def test_exact_regression_set_is_subset_of_diagnostics(self):
        all_configs = set(json.loads(suite.DEFAULT_MANIFEST.read_text()))
        exact_configs = json.loads(suite.EXACT_MANIFEST.read_text())
        self.assertEqual(len(exact_configs), 204)
        self.assertTrue(set(exact_configs).issubset(all_configs))
        for manifest in exact_configs:
            source = json.loads(suite.within_repo(manifest).read_text())["source"]
            self.assertTrue(source.startswith("vuc/src/"), source)
        for manifest in all_configs - set(exact_configs):
            source = json.loads(suite.within_repo(manifest).read_text())["source"]
            self.assertTrue(source.startswith("vuc/probes/"), source)

    def test_csu_init_and_fini_exact_same_source_and_compiler_flags(self):
        accepted = json.loads(suite.EXACT_MANIFEST.read_text())
        for member, address, size in (
            ("init", "0x08071f70", 84),
            ("fini", "0x08071fd0", 80),
        ):
            config_path = "vuc/config/libc_csu_" + member + "_pic_probe.json"
            with self.subTest(member=member):
                self.assertIn(config_path, accepted)
                config = json.loads(suite.within_repo(config_path).read_text())
                self.assertEqual(config["source"], "vuc/src/LibcCsu.c")
                self.assertEqual(config["flags"], ["-xc", "-O2", "-fPIC"])
                self.assertEqual(config["address"], address)
                self.assertEqual(config["size"], size)
                self.assertEqual(len(config["relocations"]), 4)

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

    def test_four_source_verified_unresolved_partials_are_experimental_only(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        selected = set(json.loads(suite.DEFAULT_MANIFEST.read_text()))
        partials = {
            "assemble_binop_dispatch_oct14_probe.json": ("0x0805a3f0", 193, 4),
            "translate_expr_list_oct14_probe.json": ("0x0806ae20", 256, 2),
            "canonize_fragment_oct14_probe.json": ("0x08050460", 224, 8),
            "value_addition_partial_oct14_probe.json": ("0x0806e9b0", 219, 1),
        }
        for filename, (address, length, targets) in partials.items():
            path = "vuc/config/" + filename
            with self.subTest(partial=path):
                self.assertIn(path, selected)
                self.assertNotIn(path, accepted)
                config = json.loads(suite.within_repo(path).read_text())
                self.assertEqual(config["address"], address)
                self.assertEqual(config["size"], length)
                self.assertEqual(len(config["relocations"]), targets)

    def test_improved_ast_expr_list_remains_experimental(self):
        path = "vuc/config/translate_expr_list_oct14_probe.json"
        self.assertIn(path, json.loads(suite.DEFAULT_MANIFEST.read_text()))
        self.assertNotIn(path, json.loads(suite.EXACT_MANIFEST.read_text()))
        config = json.loads(suite.within_repo(path).read_text())
        self.assertEqual(config["source"],
                         "vuc/probes/translate_expr_list_partial_improved_oct14.cpp")
        self.assertEqual(config["flags"],
                         ["-O2", "-frename-registers", "-falign-jumps=4",
                          "-fno-reorder-blocks"])
        self.assertEqual(config["size"], 256)
        self.assertEqual(config["relocations"], {
            "_Z9TranslateP7ASTExprji": "0x0806bf00",
            "_Znaj": "0x080499ac",
        })
        source = suite.within_repo(config["source"]).read_text()
        self.assertIn("int old_size=length;", source)
        self.assertIn("int index=i; ++i;", source)

    def test_improved_fragment_lists_remain_experimental(self):
        selected = set(json.loads(suite.DEFAULT_MANIFEST.read_text()))
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        for kind, address in (("var", "0x0806e0a0"),
                              ("decl", "0x0806e190")):
            path = "vuc/config/translate_frag_list_" + kind + "_oct14_probe.json"
            with self.subTest(kind=kind):
                self.assertIn(path, selected)
                self.assertNotIn(path, accepted)
                config = json.loads(suite.within_repo(path).read_text())
                self.assertEqual(config["source"],
                                 "vuc/probes/translate_frag_list_" + kind +
                                 "_partial_improved_oct14.cpp")
                self.assertEqual(config["address"], address)
                self.assertEqual(config["size"], 240)
                self.assertEqual(config["flags"],
                                 ["-O2", "-frename-registers", "-falign-jumps=4",
                                  "-fno-reorder-blocks"])
                self.assertEqual(config["relocations"], {
                    "_Z13TranslateFragP7ASTDecl": "0x0806d2c0",
                    "_Znaj": "0x080499ac",
                })
                source = suite.within_repo(config["source"]).read_text()
                self.assertIn("int old_size=length;", source)
                self.assertIn("int index=i; ++i;", source)

    def test_scope_copy_short_prefix_is_diagnostic_only(self):
        path = "vuc/config/scope_copy_prefix_diagnostic_oct14_probe.json"
        self.assertNotIn(path, json.loads(suite.EXACT_MANIFEST.read_text()))
        self.assertNotIn(path, json.loads(suite.DEFAULT_MANIFEST.read_text()))
        manifest = json.loads(suite.within_repo(path).read_text())
        self.assertEqual(manifest["source"],
                         "vuc/probes/scope_copy_119byte_partial_oct14.cpp")
        self.assertEqual(manifest["address"], "0x0804f220")
        self.assertEqual(manifest["size"], 120)
        self.assertEqual(manifest["flags"],
                         ["-O2", "-frename-registers", "-falign-jumps=8"])
        self.assertEqual(len(manifest["relocations"]), 6)

    def test_string_fragment_list_recovered_index_lifetime_is_experimental(self):
        path = "vuc/config/translate_frag_list_string_oct14_probe.json"
        self.assertNotIn(path, json.loads(suite.EXACT_MANIFEST.read_text()))
        content = suite.within_repo(
            "vuc/probes/translate_frag_list_string_partial_oct14.cpp").read_text()
        self.assertIn(
            "IR::Fragment_Data *fragment = new IR::Fragment_Data(name);\n"
            "        ++i;\n"
            "        output.Add(fragment);", content)

    def test_scope_copy_throw_declaration_is_accepted_exact(self):
        path = "vuc/config/scope_copy_exact_oct14_probe.json"
        self.assertIn(path, json.loads(suite.EXACT_MANIFEST.read_text()))
        self.assertIn(path, json.loads(suite.DEFAULT_MANIFEST.read_text()))
        manifest = json.loads(suite.within_repo(path).read_text())
        self.assertEqual(manifest["address"], "0x0804f220")
        self.assertEqual(manifest["size"], 120)
        self.assertEqual(manifest["source"], "vuc/src/ScopeUninitializedCopy.cpp")
        self.assertEqual(len(manifest["relocations"]), 6)
        self.assertIn("void clear() throw();",
                      suite.within_repo("vuc/src/ScopeCopyTable.h").read_text())

    def test_string_fragment_list_partial_is_never_accepted_as_exact(self):
        path = "vuc/config/translate_frag_list_string_oct14_probe.json"
        self.assertIn(path, json.loads(suite.DEFAULT_MANIFEST.read_text()))
        self.assertNotIn(path, json.loads(suite.EXACT_MANIFEST.read_text()))
        manifest = json.loads(suite.within_repo(path).read_text())
        self.assertEqual(manifest["address"], "0x0806e5b0")
        self.assertEqual(manifest["size"], 306)
        self.assertEqual(manifest["source"],
                         "vuc/probes/translate_frag_list_string_partial_oct14.cpp")
        self.assertEqual(manifest["flags"],
                         ["-O2", "-frename-registers", "-falign-jumps=1"])
        self.assertEqual(len(manifest["relocations"]), 6)

    def test_value_logical_or_is_exact_and_and_remains_experimental(self):
        exact = json.loads(suite.EXACT_MANIFEST.read_text())
        selected = json.loads(suite.DEFAULT_MANIFEST.read_text())
        exact_path = "vuc/config/value_logical_or_exact_oct14_probe.json"
        and_path = "vuc/config/value_logical_and_partial_oct14_probe.json"
        self.assertIn(exact_path, exact)
        self.assertIn(and_path, selected)
        self.assertNotIn(and_path, exact)
        or_config = json.loads(suite.within_repo(exact_path).read_text())
        self.assertEqual(or_config["address"], "0x0806f490")
        self.assertEqual(or_config["size"], 309)
        self.assertEqual(or_config["source"], "vuc/src/ValueLogicalOr.cpp")
        self.assertEqual(or_config["relocations"], {
            "_ZN5Value11TypeConvertERKS_S1_RS_S2_": "0x0806f7d0"})
        and_config = json.loads(suite.within_repo(and_path).read_text())
        self.assertEqual(and_config["size"], 325)
        self.assertEqual(and_config["source"],
                         "vuc/probes/value_logical_and_partial_oct14.cpp")
        self.assertEqual(and_config["flags"], [
            "-O2", "-frename-registers", "-ffast-math",
            "-falign-jumps=16", "-falign-labels=1", "-falign-loops=1"])
        and_source = suite.within_repo(and_config["source"]).read_text()
        self.assertIn("case 4: result.bits=!(", and_source)
        self.assertIn("(result.values[1] == 0.0f) ||", and_source)
        self.assertIn("(temporary.values[3] == 0.0f)", and_source)
        or_source = suite.within_repo(or_config["source"]).read_text()
        self.assertIn(")) && ((result.values[1]", or_source)

    def test_value_addition_reordered_lanes_remain_experimental(self):
        """Authentic 219-byte Value+ source has four independent vector lanes."""
        path = "vuc/config/value_addition_partial_oct14_probe.json"
        self.assertIn(path, json.loads(suite.DEFAULT_MANIFEST.read_text()))
        self.assertNotIn(path, json.loads(suite.EXACT_MANIFEST.read_text()))
        config = json.loads(suite.within_repo(path).read_text())
        self.assertEqual(config["address"], "0x0806e9b0")
        self.assertEqual(config["size"], 219)
        self.assertEqual(config["source"],
                         "vuc/probes/ValueAdditionLaneOrderOct15.cpp")
        self.assertEqual(config["flags"],
                         ["-O2", "-frename-registers", "-falign-jumps=1"])
        self.assertEqual(config["relocations"], {
            "_ZN5Value11TypeConvertERKS_S1_RS_S2_": "0x0806f7d0"})
        source = suite.within_repo(config["source"]).read_text()
        body = source[source.index("Value Value::operator+(const Value &other) const"):
                      source.index("Value Value::operator-(const Value &other) const")]
        vector_body = body[body.index("case 4:"):]
        offsets = [vector_body.index(f"result.values[{lane}] += temporary.values[{lane}];")
                   for lane in (3, 1, 2, 0)]
        self.assertEqual(offsets, sorted(offsets))

    def test_const_value_parser_is_original_length_experimental(self):
        """The recovered AST layout is a partial, not an accepted function."""
        path = "vuc/config/value_set_constval_partial_oct15b.json"
        self.assertIn(path, json.loads(suite.DEFAULT_MANIFEST.read_text()))
        self.assertNotIn(path, json.loads(suite.EXACT_MANIFEST.read_text()))
        config = json.loads(suite.within_repo(path).read_text())
        self.assertEqual(config["address"], "0x0806e7d0")
        self.assertEqual(config["size"], 455)
        self.assertEqual(config["source"],
                         "vuc/probes/ValueConstValSetPartialOct15b.cpp")
        self.assertEqual(config["flags"], [
            "-O2", "-frename-registers", "-falign-jumps=4",
            "-falign-labels=2", "-falign-loops=1", "-fno-reorder-blocks"])
        self.assertEqual(config["relocation_literals"][".rodata"], [
            {"offset": 0, "size": 5, "address": "0x08075f78"},
            {"offset": 5, "size": 6, "address": "0x0807668a"}])
        self.assertEqual(len(config["relocations"]), 5)
        source = suite.within_repo(config["source"]).read_text()
        self.assertIn("unsigned char original_ast_header[12]", source)
        self.assertIn("__strtol_internal", source)
        self.assertIn("__strtod_internal", source)
        self.assertIn("std::strrchr", source)

    def test_mltemp_default_c2_selects_original_constructor_source_order(self):
        """The genuine formatted predecessor yields a longer exact C2 prefix."""
        path = "vuc/config/ml_temp_default_C2_probe.json"
        self.assertIn(path, json.loads(suite.DEFAULT_MANIFEST.read_text()))
        self.assertNotIn(path, json.loads(suite.EXACT_MANIFEST.read_text()))
        config = json.loads(suite.within_repo(path).read_text())
        self.assertEqual(config["address"], "0x08071c20")
        self.assertEqual(config["size"], 101)
        self.assertEqual(config["source"],
                         "vuc/probes/ml_temp_constructor_unit_oct12.cpp")
        self.assertEqual(config["flags"], ["-O2"])
        self.assertEqual(len(config["relocations"]), 3)
        self.assertEqual(len(config["relocation_objects"][".bss"]), 2)
        source = suite.within_repo(config["source"]).read_text()
        self.assertLess(source.index("MLTempString::MLTempString(const char *format"),
                        source.index("MLTempString::MLTempString()"))

    def test_hash_copy_node_arguments_are_experimental_only(self):
        path = "vuc/config/hash_copy_node_args_partial_oct14_probe.json"
        self.assertIn(path, json.loads(suite.DEFAULT_MANIFEST.read_text()))
        self.assertNotIn(path, json.loads(suite.EXACT_MANIFEST.read_text()))
        config = json.loads(suite.within_repo(path).read_text())
        self.assertEqual(config["address"], "0x0804f420")
        self.assertEqual(config["size"], 166)
        self.assertEqual(len(config["relocations"]), 4)
        self.assertEqual(config["source"],
                         "vuc/probes/hash_copy_node_args_partial_oct14.cpp")
        source = suite.within_repo(config["source"]).read_text()
        self.assertIn('#include "MLHashTableCopyNodePartial.h"', source)
        self.assertIn("Node *next = node->next", source)
        self.assertIn("new Node(node->key, node->hash, node->value", source)

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
        for neighbor in ("symboltable_find_probe.json", "symbol_begin_scope_probe.json",
                         "symboltable_end_exact_probe.json", "hash_symbol_findcopy_probe.json"):
            self.assertIn("vuc/config/" + neighbor, accepted)
        findcopy = json.loads(suite.within_repo("vuc/config/hash_symbol_findcopy_probe.json").read_text())
        self.assertEqual(findcopy["source"], "vuc/src/SymbolTableUnit.cpp")
        self.assertEqual(findcopy["address"], "0x0806a8e0")
        self.assertEqual(findcopy["size"], 114)
        unit = suite.within_repo("vuc/src/SymbolTableUnit.cpp").read_text()
        header = suite.within_repo("vuc/src/MLHashTable.h").read_text()
        self.assertIn("Node(const Key &k, unsigned h, Value v)", header)
        self.assertIn("Node() {}", header)
        self.assertIn("new SymbolScope::Node(name, hasher.Hash(name), entry)", unit)
        self.assertIn("node->next = scope.heads[bucket];", unit)
        self.assertIn("scope.heads[bucket] = node;", unit)
        self.assertNotIn("struct SymbolNode", unit)
        self.assertLess(unit.index("void SymbolTable::Enter("),
                        unit.index("SymbolTableEntry *SymbolTable::Find("))
        self.assertLess(unit.index("SymbolTableEntry *SymbolTable::Find("),
                        unit.index("void SymbolTable::BeginScope()"))
        self.assertLess(unit.index("void SymbolTable::BeginScope()"),
                        unit.index("void SymbolTable::EndScope()"))
        self.assertLess(unit.index("void SymbolTable::EndScope()"),
                        unit.index("Value MLHashTable<Key, Value, Hasher, Buckets>::FindCopy"))
        for source_path in ("vuc/src/SymbolTableUnit.cpp", "vuc/src/SymbolTable.cpp"):
            source = suite.within_repo(source_path).read_text()
            self.assertIn("scopes_.pop_back();", source)
            self.assertNotIn("--scope_;", source)

    def test_scope_reserve_is_original_length_and_strictly_experimental(self):
        """A true emitted STL template matches 183/241; never accept on size."""
        path = "vuc/config/scope_vector_reserve_partial_oct15m.json"
        self.assertIn(path, json.loads(suite.DEFAULT_MANIFEST.read_text()))
        self.assertNotIn(path, json.loads(suite.EXACT_MANIFEST.read_text()))
        config = json.loads(suite.within_repo(path).read_text())
        self.assertEqual(config["address"], "0x0804ec50")
        self.assertEqual(config["size"], 241)
        self.assertEqual(config["source"],
                         "vuc/probes/ScopeVectorReserveOriginalUnitOct15m.cpp")
        self.assertIn("../src/ScopeVectorLifecycle.cpp",
                      suite.within_repo(config["source"]).read_text())
        self.assertEqual(len(config["relocations"]), 4)
        self.assertEqual(sum(x["size"] for x in config["relocation_literals"][".rodata"]), 54)

    def test_ast_fun_call_printer_has_real_source_unit_literal_ownership(self):
        path = "vuc/config/print_ast_fun_context_oct14_probe.json"
        self.assertIn(path, json.loads(suite.DEFAULT_MANIFEST.read_text()))
        self.assertNotIn(path, json.loads(suite.EXACT_MANIFEST.read_text()))
        config = json.loads(suite.within_repo(path).read_text())
        self.assertEqual(config["size"], 98)
        self.assertEqual(config["source"],
                         "vuc/probes/PrintASTFunCallRealUnitOct15m.cpp")
        self.assertIn("../src/Printer.cpp",
                      suite.within_repo(config["source"]).read_text())
        self.assertEqual(sum(x["size"] for x in config["relocation_literals"][".rodata"]),9)
        self.assertEqual(len(config["relocations"]),3)

    def test_type_stream_input_is_original_length_but_not_exact(self):
        """Preserve authenticated old-style Type parser without overclaiming."""
        path = "vuc/config/type_stream_input_partial_oct15n.json"
        self.assertIn(path, json.loads(suite.DEFAULT_MANIFEST.read_text()))
        self.assertNotIn(path, json.loads(suite.EXACT_MANIFEST.read_text()))
        config = json.loads(suite.within_repo(path).read_text())
        self.assertEqual(config["address"], "0x0804f2a0")
        self.assertEqual(config["size"], 369)
        self.assertEqual(config["source"],
                         "vuc/probes/TypeStreamInputOriginalUnitOct15n.cpp")
        source = suite.within_repo(config["source"]).read_text()
        self.assertIn("static const char *tn[]", source)
        self.assertIn("Type temporary;", source)
        self.assertEqual(len(config["relocations"]), 4)
        self.assertEqual(sum(r["size"] for r in config["relocation_literals"][".rodata"]), 48)
        self.assertEqual(config["relocation_objects"][".data"]["_ZZrsRSiR4TypeE2tn"],
                         "0x0807700c")

    def test_new_reduce_binop_specializations_remain_strict_partials(self):
        accepted=set(json.loads(suite.EXACT_MANIFEST.read_text()))
        chosen=set(json.loads(suite.DEFAULT_MANIFEST.read_text()))
        for kind,address,code in (("unsigned","0x08050540","Ij"),
                                  ("signed","0x08050680","Ii")):
            path=f"vuc/config/reduce_binop_{kind}_partial_oct15o.json"
            self.assertIn(path,chosen)
            self.assertNotIn(path,accepted)
            cfg=json.loads(suite.within_repo(path).read_text())
            self.assertEqual(cfg["address"],address)
            self.assertEqual(cfg["size"],315)
            self.assertIn(f"ReduceBinOp{code}",cfg["function"])
            self.assertEqual(cfg["source"],"vuc/probes/ReduceBinOpNewOct15o.cpp")
            self.assertEqual(cfg["relocations"],{})
        src=suite.within_repo("vuc/probes/ReduceBinOpNewOct15o.cpp").read_text()
        self.assertIn("else return true;",src)

    def test_new_concatenate_binop_is_literal_authenticated_experimental(self):
        path="vuc/config/concatenate_binop_partial_oct15o.json"
        self.assertIn(path,json.loads(suite.DEFAULT_MANIFEST.read_text()))
        self.assertNotIn(path,json.loads(suite.EXACT_MANIFEST.read_text()))
        cfg=json.loads(suite.within_repo(path).read_text())
        self.assertEqual(cfg["address"],"0x080507c0")
        self.assertEqual(cfg["size"],346)
        self.assertEqual(cfg["source"],"vuc/probes/ConcatenateBinOpNewOct15o.cpp")
        self.assertEqual(len(cfg["relocations"]),3)
        self.assertEqual(sum(x["size"] for x in cfg["relocation_literals"][".rodata"]),44)

    def test_new_canonize_try_reduce_template_partials(self):
        accepted=set(json.loads(suite.EXACT_MANIFEST.read_text()))
        chosen=set(json.loads(suite.DEFAULT_MANIFEST.read_text()))
        for kind,address,code in (("unsigned","0x0804ffb0","Ij"),
                                  ("signed","0x08050120","Ii")):
            path=f"vuc/config/canonize_try_reduce_{kind}_partial_oct15p.json"
            self.assertIn(path,chosen)
            self.assertNotIn(path,accepted)
            cfg=json.loads(suite.within_repo(path).read_text())
            self.assertEqual(cfg["address"],address)
            self.assertEqual(cfg["size"],359)
            self.assertIn(f"Canonize_TryReduceExpr{code}",cfg["function"])
            self.assertEqual(cfg["source"],"vuc/probes/CanonizeTryReduceNewOct15p.cpp")
            self.assertEqual(cfg["relocation_literals"] if "relocation_literals" in cfg else {},{})
            self.assertEqual(len(cfg["relocations"]),9)
        source=suite.within_repo("vuc/probes/CanonizeTryReduceNewOct15p.cpp").read_text()
        self.assertIn("localEnv.FindPtr(name)",source)
        self.assertIn("globalEnv.FindPtr(name)",source)

    def test_broadcast_move_original_length_and_real_source_literals(self):
        path="vuc/config/broadcast_move_partial_oct15q.json"
        self.assertIn(path,json.loads(suite.DEFAULT_MANIFEST.read_text()))
        self.assertNotIn(path,json.loads(suite.EXACT_MANIFEST.read_text()))
        config=json.loads(suite.within_repo(path).read_text())
        self.assertEqual(config["address"],"0x08051b30")
        self.assertEqual(config["size"],416)
        self.assertEqual(config["source"],"vuc/probes/BroadcastMoveCommonAddOct15q.cpp")
        self.assertEqual(len(config["relocations"]),5)
        self.assertEqual(sum(x["size"] for x in config["relocation_literals"][".rodata"]),47)
        source=suite.within_repo(config["source"]).read_text()
        self.assertIn("mask != 15 && mask != 0",source)
        self.assertIn("MLTempString fourth(\"\")",source)

    def test_translate_type_real_ir_vtable_partial(self):
        path="vuc/config/translate_type_partial_oct15q.json"
        self.assertIn(path,json.loads(suite.DEFAULT_MANIFEST.read_text()))
        self.assertNotIn(path,json.loads(suite.EXACT_MANIFEST.read_text()))
        cfg=json.loads(suite.within_repo(path).read_text())
        self.assertEqual(cfg["address"],"0x0806a9b0")
        self.assertEqual(cfg["size"],352)
        self.assertEqual(cfg["source"],"vuc/probes/TranslateTypeFullOct15q.cpp")
        self.assertEqual(len(cfg["relocations"]),6)
        self.assertEqual(cfg["relocations"]["_ZTVN2IR10Array_DataE"],"0x08076178")
        source=suite.within_repo(cfg["source"]).read_text()
        self.assertIn("struct UInt_Data : Type_Data",source)
        self.assertIn("struct Float4_Data : Type_Data",source)
        self.assertIn("switch (kind)",source)

    def test_bind_ast_fun_call_near_exact_diagnostic_not_promoted(self):
        # Its 304-byte object resolves 23 relocations and three original
        # strings, but 16 compiler-inserted literal-padding bytes still lack
        # genuine original TU ownership. Keep both manifests strict.
        path="vuc/config/bind_ast_fun_call_partial_oct15r.json"
        self.assertNotIn(path,json.loads(suite.DEFAULT_MANIFEST.read_text()))
        self.assertNotIn(path,json.loads(suite.EXACT_MANIFEST.read_text()))
        cfg=json.loads(suite.within_repo(path).read_text())
        self.assertEqual(cfg["address"],"0x0804aad0")
        self.assertEqual(cfg["size"],304)
        self.assertEqual(cfg["source"],"vuc/probes/BindASTFunCallFreshOct15r.cpp")
        self.assertEqual(sum(x["size"] for x in cfg["relocation_literals"][".rodata"]),65)
        src=suite.within_repo(cfg["source"]).read_text()
        self.assertIn("std::cout << currFileName;",src)
        self.assertIn("matchStack.entries[matchStack.length-1]->line",src)

    def test_asm_output_print_instruction_true_literal_partial(self):
        path="vuc/config/asm_output_print_instruction_real_unit_oct15r.json"
        self.assertIn(path,json.loads(suite.DEFAULT_MANIFEST.read_text()))
        self.assertNotIn(path,json.loads(suite.EXACT_MANIFEST.read_text()))
        self.assertNotIn("vuc/config/asm_output_print_instruction_partial_oct15r.json",
                         json.loads(suite.DEFAULT_MANIFEST.read_text()))
        cfg=json.loads(suite.within_repo(path).read_text())
        self.assertEqual(cfg["address"],"0x08049c70")
        self.assertEqual(cfg["size"],382)
        self.assertEqual(cfg["source"],"vuc/probes/AsmOutputPrintInstructionOriginalUnitOct15r.cpp")
        self.assertEqual(len(cfg["relocations"]),5)
        self.assertEqual(sum(r["size"] for r in cfg["relocation_literals"][".rodata"]),154)
        source=suite.within_repo(cfg["source"]).read_text()
        self.assertIn('../src/AsmOutputUnit.cpp',source)
        accepted=suite.within_repo("vuc/src/AsmOutputUnit.cpp").read_text()
        self.assertIn("MLGameString text;",accepted)
        self.assertIn("int kind;",accepted)
        self.assertIn('else if (std::strstr(text,".equ")) isConstant=true;',source)
        self.assertIn("hex[0]='H'",source)

    def test_type_vector_insert_original_template_order_partial(self):
        path="vuc/config/type_vector_insert_aux_partial_oct15s.json"
        self.assertIn(path,json.loads(suite.DEFAULT_MANIFEST.read_text()))
        self.assertNotIn(path,json.loads(suite.EXACT_MANIFEST.read_text()))
        cfg=json.loads(suite.within_repo(path).read_text())
        self.assertEqual(cfg["address"],"0x0804ee10")
        self.assertEqual(cfg["size"],525)
        self.assertEqual(cfg["source"],"vuc/probes/TypeVectorInsertAuxRealOrderOct15s.cpp")
        self.assertEqual(len(cfg["relocations"]),7)
        source=suite.within_repo(cfg["source"]).read_text()
        insert=source.index("template void TypeVector::_M_insert_aux")
        const=source.index("template Type *std::__uninitialized_copy_aux")
        mutable=source.index("template TypeIterator std::__uninitialized_copy_aux")
        self.assertLess(insert,const)
        self.assertLess(const,mutable)

    def test_new_scope_and_string_uninitialized_templates_are_not_promoted(self):
        # Natural GCC 3.3 template instantiations recover authentic
        # algorithms but not their 149/668 original lengths yet.
        chosen=set(json.loads(suite.DEFAULT_MANIFEST.read_text()))
        accepted=set(json.loads(suite.EXACT_MANIFEST.read_text()))
        for source in ("vuc/probes/ScopeNormalIteratorUninitializedFreshOct15s.cpp",
                       "vuc/probes/StringVectorInsertAuxRealOrderOct15s.cpp",
                       "vuc/probes/StringVectorInsertAuxReversedOct15s.cpp"):
            self.assertTrue(suite.within_repo(source).is_file())
            self.assertFalse(any(json.loads(suite.within_repo(item).read_text()).get("source")==source
                                 for item in chosen | accepted))

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

    def test_script_error_real_unit_accounts_for_currfile_empty_literal(self):
        """The real compiler unit contributes one authenticated extra NUL."""
        config=json.loads(suite.within_repo("vuc/config/script_error_probe.json").read_text())
        self.assertEqual(config["source"], "vuc/probes/ScriptErrorRealUnitOct15k.cpp")
        owner=suite.within_repo(config["source"]).read_text()
        self.assertIn('#include "../src/VucCompilerUnit.cpp"',owner)
        literals=config["relocation_literals"][".rodata"]
        self.assertEqual([(x["offset"],x["size"]) for x in literals],
                         [(0,1),(1,2),(3,3)])
        currfile=json.loads(suite.within_repo("vuc/config/data_currFileName.json").read_text())
        empty=currfile["relocations"]["0x0"]
        self.assertEqual(empty["literal_hex"],"00")
        self.assertEqual(literals[0]["address"],empty["address"])
        self.assertEqual(len(config["relocations"]),9)

    def test_flex_create_buffer_and_restart_share_exact_source_context(self):
        """The authentic preceding lexer method fixes GCC register lifetime."""
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        path = "vuc/config/yy_create_buffer_probe.json"
        self.assertIn(path, accepted)
        config = json.loads(suite.within_repo(path).read_text())
        self.assertEqual(config["address"], "0x080617b0")
        self.assertEqual(config["size"], 139)
        self.assertEqual(config["flags"], ["-O2", "-frename-registers"])
        self.assertEqual(config["source"], "vuc/src/FlexLexerCreateBufferUnit.cpp")
        source = suite.within_repo(config["source"]).read_text()
        self.assertLess(source.index("void yyFlexLexer::yyrestart("),
                        source.index("yy_buffer_state *yyFlexLexer::yy_create_buffer("))
        self.assertIn('LexerError("out of dynamic memory in yy_create_buffer()")', source)

    def test_accumulatorable_exact_negative_guard_preserves_branch_orientation(self):
        """The original 77-byte emitter is now accepted from true source."""
        path = "vuc/config/ir_accumulatorable_probe.json"
        self.assertIn(path, json.loads(suite.EXACT_MANIFEST.read_text()))
        config = json.loads(suite.within_repo(path).read_text())
        self.assertEqual(config["address"], "0x0805a570")
        self.assertEqual(config["size"], 77)
        self.assertEqual(config["source"], "vuc/src/IRAccumulatorableExact.cpp")
        self.assertEqual(config["relocations"], {})
        self.assertEqual(config["flags"],
                         ["-O2", "-frename-registers", "-falign-jumps=8"])
        source = suite.within_repo(config["source"]).read_text()
        self.assertIn("if (op->operand->kind != 3) goto next_case11;", source)

    def test_get_next_word_original_length_experimental_byte_guard_source(self):
        """Only original-sized, relocation-backed scanner source is selected."""
        path = "vuc/config/get_next_word_partial_oct15e_probe.json"
        self.assertIn(path, json.loads(suite.DEFAULT_MANIFEST.read_text()))
        self.assertNotIn(path, json.loads(suite.EXACT_MANIFEST.read_text()))
        config = json.loads(suite.within_repo(path).read_text())
        self.assertEqual(config["address"], "0x080511e0")
        self.assertEqual(config["size"], 135)
        self.assertEqual(config["source"],
                         "vuc/probes/GetNextWordNestedRangePartialOct15f.cpp")
        self.assertEqual(config["flags"],
                         ["-O2", "-frename-registers", "-falign-jumps=16",
                          "-falign-labels=1", "-falign-loops=2"])
        self.assertEqual(config["relocations"], {
            "currentPosition": "0x0808bc68",
            "wordPosition": "0x0808bc70",
            "word": "0x08077440",
        })
        source = suite.within_repo(config["source"]).read_text()
        self.assertIn("if (c <= 10)", source)
        self.assertIn("if(c==44)goto endword;", source)
        self.assertIn("if(c<=44)", source)
        self.assertIn("if(c>9){if(c==32)goto endword;}", source)

    def test_get_next_word_exact_length_probe_retains_prior_source_evidence(self):
        """Keep the unselected 36-byte CFG owner for reproducible sweeps."""
        baseline = suite.within_repo(
            "vuc/probes/GetNextWordRange36BaselineOct15f.cpp").read_text()
        selected = suite.within_repo(
            "vuc/probes/GetNextWordNestedRangePartialOct15f.cpp").read_text()
        self.assertIn("if(c>44)", baseline)
        self.assertIn("if(c<=44)", selected)
        self.assertNotEqual(selected, baseline)
        for source in (selected, baseline):
            self.assertIn("if(c==44)goto endword;", source)
            self.assertIn("currentPosition=position;", source)
            self.assertIn("wordPosition=position;", source)

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
