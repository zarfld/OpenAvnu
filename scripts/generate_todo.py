#!/usr/bin/env python3
"""
OpenAvnu TODO List Generator

This script automatically generates TODO items from compliance reports
and other project tracking sources.
"""

import os
import re
from datetime import datetime, timedelta
from pathlib import Path

def generate_todo_from_compliance(compliance_file):
    """Generate TODO items from compliance report"""
    todos = []
    
    if not os.path.exists(compliance_file):
        return todos
    
    with open(compliance_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Extract critical errors
    critical_section = re.search(r'## Critical Errors.*?(?=##|$)', content, re.DOTALL)
    if critical_section:
        errors = re.findall(r'- ERROR: (.*)', critical_section.group(0))
        for error in errors:
            todos.append({
                'priority': 'P0',
                'type': 'Critical',
                'description': f"Fix: {error}",
                'due_date': (datetime.now() + timedelta(days=2)).strftime('%Y-%m-%d'),
                'owner': 'TBD',
                'status': 'Not Started'
            })
    
    # Extract warnings
    warning_section = re.search(r'## Warnings.*?(?=##|$)', content, re.DOTALL)
    if warning_section:
        warnings = re.findall(r'- WARNING: (.*)', warning_section.group(0))
        for warning in warnings:
            todos.append({
                'priority': 'P1',
                'type': 'Warning',
                'description': f"Fix: {warning}",
                'due_date': (datetime.now() + timedelta(days=7)).strftime('%Y-%m-%d'),
                'owner': 'TBD',
                'status': 'Not Started'
            })
    
    return todos

def generate_hardware_todos():
    """Generate hardware validation TODO items"""
    return [
        {
            'priority': 'P0',
            'type': 'Hardware',
            'description': 'Test Intel i210 NIC on Windows 10 - clock synchronization',
            'due_date': (datetime.now() + timedelta(days=30)).strftime('%Y-%m-%d'),
            'owner': 'TBD',
            'status': 'Blocked - Need hardware'
        },
        {
            'priority': 'P0',
            'type': 'Hardware',
            'description': 'Test Intel i210 NIC on Windows 11 - clock synchronization',
            'due_date': (datetime.now() + timedelta(days=30)).strftime('%Y-%m-%d'),
            'owner': 'TBD',
            'status': 'Blocked - Need hardware'
        },
        {
            'priority': 'P0',
            'type': 'Hardware',
            'description': 'Test Intel i219 NIC on Windows 10 - clock synchronization',
            'due_date': (datetime.now() + timedelta(days=30)).strftime('%Y-%m-%d'),
            'owner': 'TBD',
            'status': 'Blocked - Need hardware'
        },
        {
            'priority': 'P0',
            'type': 'Hardware',
            'description': 'Test Intel i219 NIC on Windows 11 - clock synchronization',
            'due_date': (datetime.now() + timedelta(days=30)).strftime('%Y-%m-%d'),
            'owner': 'TBD',
            'status': 'Blocked - Need hardware'
        }
    ]

def generate_todo_markdown(todos):
    """Generate markdown TODO list"""
    md_content = []
    md_content.append("# OpenAvnu Auto-Generated TODO List")
    md_content.append(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    md_content.append("")
    
    # Group by priority
    priorities = {'P0': [], 'P1': [], 'P2': [], 'P3': []}
    for todo in todos:
        priority = todo.get('priority', 'P3')
        if priority in priorities:
            priorities[priority].append(todo)
    
    priority_names = {
        'P0': '🚨 Critical Priority',
        'P1': '⚠️ High Priority', 
        'P2': '📋 Medium Priority',
        'P3': '💡 Low Priority'
    }
    
    for priority in ['P0', 'P1', 'P2', 'P3']:
        if priorities[priority]:
            md_content.append(f"## {priority_names[priority]}")
            md_content.append("")
            
            for todo in priorities[priority]:
                status_emoji = {
                    'Not Started': '🔴',
                    'In Progress': '🟡',
                    'Completed': '🟢',
                    'Blocked': '🔵'
                }.get(todo.get('status', 'Not Started'), '🔴')
                
                md_content.append(f"- [ ] **{todo['description']}**")
                md_content.append(f"  - Status: {status_emoji} {todo.get('status', 'Not Started')}")
                md_content.append(f"  - Owner: {todo.get('owner', 'TBD')}")
                md_content.append(f"  - Due: {todo.get('due_date', 'TBD')}")
                md_content.append(f"  - Type: {todo.get('type', 'General')}")
                md_content.append("")
    
    md_content.append("## 📊 Summary")
    md_content.append(f"- Total Tasks: {len(todos)}")
    md_content.append(f"- Critical (P0): {len(priorities['P0'])}")
    md_content.append(f"- High (P1): {len(priorities['P1'])}")
    md_content.append(f"- Medium (P2): {len(priorities['P2'])}")
    md_content.append(f"- Low (P3): {len(priorities['P3'])}")
    md_content.append("")
    md_content.append("---")
    md_content.append("*This file is auto-generated. Do not edit manually.*")
    md_content.append(f"*Run `python scripts/generate_todo.py` to update.*")
    
    return '\n'.join(md_content)

def main():
    """Main function"""
    repo_path = Path('.')
    docs_path = repo_path / 'docs'
    
    # Generate TODOs from various sources
    todos = []
    
    # From compliance report
    compliance_file = docs_path / 'compliance_report.md'
    todos.extend(generate_todo_from_compliance(compliance_file))
    
    # Hardware validation tasks
    todos.extend(generate_hardware_todos())
    
    # Generate markdown
    todo_markdown = generate_todo_markdown(todos)
    
    # Write to file
    output_file = docs_path / 'TODO_AUTO_GENERATED.md'
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(todo_markdown)
    
    print(f"Generated TODO list with {len(todos)} items")
    print(f"Written to: {output_file}")

if __name__ == "__main__":
    main()
