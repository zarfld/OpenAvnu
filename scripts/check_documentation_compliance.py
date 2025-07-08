#!/usr/bin/env python3
"""
OpenAvnu Documentation Compliance Checker

This script validates documentation against the governance rules defined in
DOCUMENTATION_GOVERNANCE.md
"""

import os
import re
import sys
from pathlib import Path
from datetime import datetime, timedelta
import argparse

class DocumentationChecker:
    def __init__(self, repo_path):
        self.repo_path = Path(repo_path)
        self.docs_path = self.repo_path / "docs"
        self.errors = []
        self.warnings = []
        
        # Prohibited terms without hardware validation
        self.prohibited_terms = [
            "PRODUCTION READY",
            "CERTIFIED",
            "MEETS ALL REQUIREMENTS", 
            "FULLY VALIDATED",
            "COMPLETE IMPLEMENTATION",
            "READY FOR DEPLOYMENT"
        ]
        
        # Required hardware validation evidence
        self.hardware_evidence_required = [
            "Intel i210",
            "Intel i219", 
            "hardware timestamping",
            "clock synchronization"
        ]
        
    def check_file_naming(self):
        """Check if files follow naming conventions"""
        naming_patterns = {
            "completed": r".*_IMPLEMENTATION\.md$",
            "status": r".*_STATUS\.md$", 
            "work-in-progress": r".*_DEVELOPMENT\.md$",
            "archive": r".*_ARCHIVED_\d{4}-\d{2}-\d{2}\.md$"
        }
        
        for folder, pattern in naming_patterns.items():
            folder_path = self.docs_path / folder
            if folder_path.exists():
                for file in folder_path.rglob("*.md"):
                    if not re.match(pattern, file.name):
                        self.warnings.append(f"Naming convention violation: {file.relative_to(self.repo_path)}")
    
    def check_prohibited_terms(self):
        """Check for prohibited terms without hardware validation"""
        for md_file in self.docs_path.rglob("*.md"):
            try:
                with open(md_file, 'r', encoding='utf-8') as f:
                    content = f.read()
                    
                for term in self.prohibited_terms:
                    if term in content:
                        # Check if hardware validation evidence exists
                        has_evidence = any(evidence in content for evidence in self.hardware_evidence_required)
                        if not has_evidence:
                            self.errors.append(
                                f"Prohibited term '{term}' found without hardware validation evidence in {md_file.relative_to(self.repo_path)}"
                            )
            except Exception as e:
                self.warnings.append(f"Could not read {md_file.relative_to(self.repo_path)}: {e}")
    
    def check_required_sections(self):
        """Check if required sections are present in documentation"""
        required_sections = {
            "completed": ["Hardware configuration tested", "Test results", "Validation date"],
            "status": ["Current status", "Recent progress", "Next steps"],
            "work-in-progress": ["Current development status", "Remaining tasks", "Target completion"]
        }
        
        for folder, sections in required_sections.items():
            folder_path = self.docs_path / folder
            if folder_path.exists():
                for file in folder_path.rglob("*.md"):
                    try:
                        with open(file, 'r', encoding='utf-8') as f:
                            content = f.read()
                            
                        for section in sections:
                            if section.lower() not in content.lower():
                                self.warnings.append(
                                    f"Missing required section '{section}' in {file.relative_to(self.repo_path)}"
                                )
                    except Exception as e:
                        self.warnings.append(f"Could not read {file.relative_to(self.repo_path)}: {e}")
    
    def check_status_freshness(self):
        """Check if status documents are up to date (within 45 days)"""
        status_path = self.docs_path / "status"
        if status_path.exists():
            cutoff_date = datetime.now() - timedelta(days=45)
            
            for file in status_path.rglob("*.md"):
                try:
                    stat = file.stat()
                    mod_time = datetime.fromtimestamp(stat.st_mtime)
                    
                    if mod_time < cutoff_date:
                        self.warnings.append(
                            f"Status document may be outdated (last modified {mod_time.strftime('%Y-%m-%d')}): {file.relative_to(self.repo_path)}"
                        )
                except Exception as e:
                    self.warnings.append(f"Could not check modification time for {file.relative_to(self.repo_path)}: {e}")
    
    def check_cross_references(self):
        """Check for broken internal links"""
        for md_file in self.docs_path.rglob("*.md"):
            try:
                with open(md_file, 'r', encoding='utf-8') as f:
                    content = f.read()
                    
                # Find markdown links
                links = re.findall(r'\[([^\]]+)\]\(([^)]+)\)', content)
                for link_text, link_url in links:
                    if link_url.startswith(('http://', 'https://')):
                        continue  # Skip external links
                        
                    # Check if internal link exists
                    if link_url.startswith('/'):
                        target_path = self.repo_path / link_url[1:]
                    else:
                        target_path = md_file.parent / link_url
                        
                    if not target_path.exists():
                        self.warnings.append(
                            f"Broken link '{link_url}' in {md_file.relative_to(self.repo_path)}"
                        )
            except Exception as e:
                self.warnings.append(f"Could not check links in {md_file.relative_to(self.repo_path)}: {e}")
    
    def generate_compliance_report(self):
        """Generate a comprehensive compliance report"""
        report = []
        report.append("# OpenAvnu Documentation Compliance Report")
        report.append(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        report.append("")
        
        if self.errors:
            report.append("## 🚨 Critical Errors (Must Fix)")
            for error in self.errors:
                report.append(f"- ❌ {error}")
            report.append("")
        
        if self.warnings:
            report.append("## ⚠️ Warnings (Should Fix)")
            for warning in self.warnings:
                report.append(f"- ⚠️ {warning}")
            report.append("")
        
        if not self.errors and not self.warnings:
            report.append("## ✅ All Checks Passed")
            report.append("Documentation is compliant with governance rules.")
        
        report.append("")
        report.append("## 📊 Summary")
        report.append(f"- Critical Errors: {len(self.errors)}")
        report.append(f"- Warnings: {len(self.warnings)}")
        report.append(f"- Total Issues: {len(self.errors) + len(self.warnings)}")
        
        return "\n".join(report)
    
    def run_all_checks(self):
        """Run all compliance checks"""
        print("Running documentation compliance checks...")
        
        self.check_file_naming()
        self.check_prohibited_terms()
        self.check_required_sections()
        self.check_status_freshness()
        self.check_cross_references()
        
        return len(self.errors) == 0  # Return True if no critical errors

def main():
    parser = argparse.ArgumentParser(description='Check OpenAvnu documentation compliance')
    parser.add_argument('--repo-path', default='.', help='Path to OpenAvnu repository')
    parser.add_argument('--output', help='Output file for compliance report')
    parser.add_argument('--fail-on-warnings', action='store_true', help='Exit with error code if warnings found')
    
    args = parser.parse_args()
    
    checker = DocumentationChecker(args.repo_path)
    success = checker.run_all_checks()
    
    report = checker.generate_compliance_report()
    
    if args.output:
        with open(args.output, 'w', encoding='utf-8') as f:
            f.write(report)
        print(f"Compliance report written to {args.output}")
    else:
        print(report)
    
    # Exit with appropriate code
    if not success:
        sys.exit(1)  # Critical errors found
    elif args.fail_on_warnings and checker.warnings:
        sys.exit(2)  # Warnings found and fail-on-warnings enabled
    else:
        sys.exit(0)  # All good

if __name__ == "__main__":
    main()
