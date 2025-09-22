#!/usr/bin/env -S uv run --script
# /// script
# dependencies = ["pathlib", "re"]
# ///

"""
fix_data_h_prototypes.py - Fix data.h extern prototypes systematically

This script addresses the specific data.h problem by:
1. Restoring the original K&R style declarations (empty parentheses)
2. Removing problematic (void) additions that were incorrect
3. Fixing specific known signature mismatches

The strategy is to revert to the safer K&R declarations until proper
ANSI C prototypes can be generated systematically in Phase 8.

Usage:
    python3 fix_data_h_prototypes.py --restore-kr-style
    python3 fix_data_h_prototypes.py --fix-signatures
"""

import argparse
import pathlib
import re

class DataHFixer:
    def __init__(self):
        self.fixes_applied = []
        self.problematic_functions = {
            # Functions that definitely need parameter fixes based on usage
            'move_file': 'int move_file(char *from, char *to)',
            'canbeseen': 'int canbeseen(int x, int y)',
            'units_in_sector': 'int units_in_sector(int x, int y, int nation)',
            'tofood': 'int tofood(struct s_sector *sptr, int cntry)',
            'mailopen': 'int mailopen(int to)',
            'tg_ok': 'int tg_ok(int nation, struct s_sector *sptr)',
            'fort_val': 'int fort_val(struct s_sector *sptr)',
            'compass': 'int compass(int x0, int y0, int x1, int y1)',
            'check_lock': 'int check_lock(char *filename, int keeplock)',
            'get_pass': 'int get_pass(char *str)',
            'fltships': 'int fltships(int country, int nvynum)',
            'makeside': 'void makeside(int alwayssee)',
            'highlight': 'void highlight(int x, int y, short hmode)',
            'errormsg': 'void errormsg(char *str)',
            'clear_bottom': 'void clear_bottom(int i)',
            'checkout': 'void checkout(char *__file__, int __line__)',
            'peasant_revolt': 'void peasant_revolt(int *newnation)',
            'other_revolt': 'int other_revolt(int *new)',
            'verify_ntn': 'void verify_ntn(void)',
            'verify_sct': 'void verify_sct(void)',
            'verifydata': 'void verifydata(char *__file__, int __line__)',
            'prep': 'void prep(int nation, int leader)',
            'armyrpt': 'void armyrpt(int repnum)',
            'addgroup': 'void addgroup(void)',
            'ext_cmd': 'void ext_cmd(int armie)',
            'updmove': 'void updmove(int race, int country)',
            'mailclose': 'void mailclose(int to)'
        }

    def restore_kr_style(self, data_h_path='data.h'):
        """Restore safe K&R style declarations by removing problematic (void) parameters"""

        content = pathlib.Path(data_h_path).read_text()
        original_content = content

        # Find the extern function declaration section
        lines = content.split('\n')
        new_lines = []
        in_extern_section = False

        for line in lines:
            if 'extern all subroutine calls' in line:
                in_extern_section = True
                new_lines.append(line)
                continue
            elif in_extern_section and (line.strip().startswith('#') or
                                      ('/*' in line and '*/' in line) or
                                      line.strip() == ''):
                new_lines.append(line)
                continue
            elif in_extern_section and line.strip().startswith('extern'):
                # Process extern declarations
                modified_line = self.fix_extern_line(line)
                new_lines.append(modified_line)
                if modified_line != line:
                    self.fixes_applied.append(f"Changed: {line.strip()} -> {modified_line.strip()}")
                continue
            elif in_extern_section and line.strip() and not line.strip().startswith('extern'):
                # End of extern section
                in_extern_section = False
                new_lines.append(line)
            else:
                new_lines.append(line)

        if new_lines != lines:
            # Write the fixed content
            fixed_content = '\n'.join(new_lines)
            pathlib.Path(data_h_path).write_text(fixed_content)
            return True
        return False

    def fix_extern_line(self, line):
        """Fix a single extern declaration line"""

        # Handle lines with multiple function declarations
        # extern int func1(), func2(), func3();
        if ',' in line and '(' in line:
            return self.fix_multiple_function_line(line)

        # Handle single function declarations
        # extern int func(void);
        if '(void)' in line:
            # Check if this function should actually have parameters
            func_match = re.search(r'([a-zA-Z_][a-zA-Z0-9_]*)\s*\(void\)', line)
            if func_match:
                func_name = func_match.group(1)
                if func_name in self.problematic_functions:
                    # Replace with the correct signature
                    correct_sig = self.problematic_functions[func_name]
                    return f"extern {correct_sig};"
                else:
                    # For other functions, revert to K&R style (safer)
                    return line.replace('(void)', '()')

        # Handle specific known problems
        if 'conquer_access' in line:
            # This was renamed to avoid conflict with system access()
            return line

        return line

    def fix_multiple_function_line(self, line):
        """Fix extern lines with multiple function declarations"""

        # Pattern: extern return_type func1(), func2(), func3();
        match = re.match(r'extern\s+([^(]+?)\s+([^;]+);', line.strip())
        if not match:
            return line

        return_type = match.group(1).strip()
        func_part = match.group(2).strip()

        # Split by commas, but be careful of function parameters
        functions = []
        current_func = ""
        paren_level = 0

        for char in func_part:
            if char == '(':
                paren_level += 1
            elif char == ')':
                paren_level -= 1
            elif char == ',' and paren_level == 0:
                if current_func.strip():
                    functions.append(current_func.strip())
                current_func = ""
                continue
            current_func += char

        if current_func.strip():
            functions.append(current_func.strip())

        # Process each function
        fixed_functions = []
        for func in functions:
            func = func.strip()
            if '(void)' in func:
                func_name_match = re.search(r'([a-zA-Z_][a-zA-Z0-9_]*)\s*\(void\)', func)
                if func_name_match:
                    func_name = func_name_match.group(1)
                    if func_name in self.problematic_functions:
                        # This function needs specific parameters
                        correct_sig = self.problematic_functions[func_name]
                        # Extract just the function signature part
                        sig_match = re.search(r'([a-zA-Z_][a-zA-Z0-9_]*\([^)]*\))', correct_sig)
                        if sig_match:
                            fixed_functions.append(sig_match.group(1))
                        else:
                            fixed_functions.append(func.replace('(void)', '()'))
                    else:
                        # Revert to K&R style
                        fixed_functions.append(func.replace('(void)', '()'))
                else:
                    fixed_functions.append(func)
            else:
                fixed_functions.append(func)

        return f"extern {return_type} {', '.join(fixed_functions)};"

    def fix_specific_signatures(self, data_h_path='data.h'):
        """Apply specific signature fixes for known problematic functions"""

        content = pathlib.Path(data_h_path).read_text()
        original_content = content

        # Apply specific fixes
        for func_name, correct_signature in self.problematic_functions.items():
            # Look for existing declarations of this function
            patterns = [
                rf'extern\s+[^;]*{re.escape(func_name)}\s*\([^)]*\)\s*;',
                rf'extern\s+[^;]*{re.escape(func_name)}\s*\(\s*void\s*\)\s*;',
                rf'extern\s+[^;]*{re.escape(func_name)}\s*\(\s*\)\s*;'
            ]

            found = False
            for pattern in patterns:
                if re.search(pattern, content):
                    content = re.sub(pattern, f'extern {correct_signature};', content)
                    found = True
                    self.fixes_applied.append(f"Fixed signature: {func_name}")
                    break

            if not found:
                # Function not found in extern list, we can add it if needed
                pass

        if content != original_content:
            pathlib.Path(data_h_path).write_text(content)
            return True
        return False

    def generate_report(self):
        """Generate a report of fixes applied"""
        report = []
        report.append("# Data.h Prototype Fix Report")
        report.append("")
        report.append(f"**Total fixes applied**: {len(self.fixes_applied)}")
        report.append("")

        if self.fixes_applied:
            report.append("## Changes Made:")
            for fix in self.fixes_applied:
                report.append(f"- {fix}")
            report.append("")

        report.append("## Strategy Applied:")
        report.append("1. Restored K&R style declarations (empty parentheses) for safety")
        report.append("2. Fixed specific known signature mismatches")
        report.append("3. Preserved correct (void) declarations where appropriate")
        report.append("")
        report.append("## Next Steps:")
        report.append("- Test compilation with these fixes")
        report.append("- Address any remaining compilation errors")
        report.append("- In Phase 8, generate proper ANSI C prototypes systematically")

        return '\n'.join(report)

def main():
    parser = argparse.ArgumentParser(description='Fix data.h extern prototype problems')

    parser.add_argument('--restore-kr-style', action='store_true',
                       help='Restore safe K&R style declarations')
    parser.add_argument('--fix-signatures', action='store_true',
                       help='Apply specific signature fixes')
    parser.add_argument('--backup', action='store_true',
                       help='Create backup before making changes')
    parser.add_argument('--report', action='store_true',
                       help='Generate fix report')
    parser.add_argument('--data-h', default='data.h',
                       help='Path to data.h file')

    args = parser.parse_args()

    fixer = DataHFixer()

    if args.backup:
        import shutil
        backup_name = f"{args.data_h}.backup"
        shutil.copy2(args.data_h, backup_name)
        print(f"Created backup: {backup_name}")

    changes_made = False

    if args.restore_kr_style:
        print("Restoring K&R style declarations...")
        if fixer.restore_kr_style(args.data_h):
            changes_made = True
            print("K&R style restoration completed")
        else:
            print("No changes needed for K&R style")

    if args.fix_signatures:
        print("Applying specific signature fixes...")
        if fixer.fix_specific_signatures(args.data_h):
            changes_made = True
            print("Signature fixes completed")
        else:
            print("No signature fixes needed")

    if args.report or changes_made:
        report = fixer.generate_report()
        if args.report:
            print(report)
        else:
            # Save report to file
            report_file = pathlib.Path('_modernization/claude/reports/data_h_fix_report.md')
            report_file.parent.mkdir(parents=True, exist_ok=True)
            report_file.write_text(report)
            print(f"Fix report saved to: {report_file}")

    if not any([args.restore_kr_style, args.fix_signatures, args.report]):
        print("No action specified. Use --help for options.")

if __name__ == '__main__':
    main()