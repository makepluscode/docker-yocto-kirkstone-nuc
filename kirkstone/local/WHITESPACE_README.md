# Automatic Whitespace Removal System

This project includes an automated system for removing whitespace issues from source code files. The system consists of a main script and a Git pre-commit hook to ensure consistent code formatting.

## Components

### 1. `remove_whitespace.sh` - Main Script
The primary script that performs whitespace cleanup on all relevant files in the project.

**Features:**
- Removes trailing whitespace from lines
- Converts line endings to Unix format (LF)
- Normalizes consecutive blank lines (keeps max 2)
- Skips binary files automatically
- Provides colored output for better visibility
- Supports dry-run mode for testing

### 2. `.git/hooks/pre-commit` - Git Hook
Automatically runs the whitespace removal script before each commit, ensuring all committed code is properly formatted.

### 3. `.whitespace-config` - Configuration File
Configurable settings for the whitespace removal process.

## Usage

### Manual Execution

Run the script manually to clean up whitespace:

```bash
# Clean all files
./remove_whitespace.sh

# Dry run (show what would be changed without modifying files)
./remove_whitespace.sh --dry-run

# Verbose output
./remove_whitespace.sh --verbose
```

### Automatic Execution (Git Pre-commit Hook)

The script runs automatically before each commit. When you run `git commit`, the pre-commit hook will:

1. Run the whitespace removal script
2. Add any modified files to the commit
3. Continue with the commit if successful

### Configuration

Edit `.whitespace-config` to customize the behavior:

```bash
# File types to process
FILE_EXTENSIONS="*.cpp *.h *.c *.qml *.sh *.md *.xml *.conf *.service"

# Directories to exclude
EXCLUDE_DIRS="build oe-workdir oe-logs .git .claude"

# Maximum blank lines to keep
MAX_BLANK_LINES=2
```

## Supported File Types

The script processes the following file types:
- **C/C++**: `.cpp`, `.h`, `.c`
- **QML**: `.qml`
- **Shell scripts**: `.sh`
- **Build files**: `CMakeLists.txt`
- **Documentation**: `.md`
- **Configuration**: `.xml`, `.conf`, `.service`

## What Gets Cleaned

### 1. Trailing Whitespace
Removes spaces and tabs at the end of lines:
```cpp
// Before
int main() {
    return 0;
}

// After
int main() {
    return 0;
}
```

### 2. Line Endings
Converts Windows line endings (CRLF) to Unix line endings (LF):
```cpp
// Before (Windows)
int main() {\r\n
    return 0;\r\n
}

// After (Unix)
int main() {
    return 0;
}
```

### 3. Blank Lines
Normalizes consecutive blank lines to a maximum of 2:
```cpp
// Before
int main() {




    return 0;




}

// After
int main() {

    return 0;

}
```

## Safety Features

- **Dry-run mode**: Test changes without modifying files
- **Binary file detection**: Automatically skips binary files
- **Backup creation**: Original files are preserved during processing
- **Error handling**: Script stops on errors to prevent data loss
- **Git integration**: Changes are automatically staged for commit

## Troubleshooting

### Script Not Executable
```bash
chmod +x remove_whitespace.sh
```

### Pre-commit Hook Not Working
```bash
chmod +x .git/hooks/pre-commit
```

### Skip Pre-commit Hook (Emergency)
```bash
git commit --no-verify -m "Emergency commit"
```

### Check What Files Would Be Modified
```bash
./remove_whitespace.sh --dry-run
```

## Integration with IDEs

### VS Code
Add to your workspace settings (`.vscode/settings.json`):
```json
{
    "files.trimTrailingWhitespace": true,
    "files.insertFinalNewline": true,
    "files.trimFinalNewlines": true
}
```

### Qt Creator
Enable "Remove trailing whitespace" in Tools → Options → Text Editor → Behavior.

## Best Practices

1. **Run regularly**: Use the script before committing or as part of your development workflow
2. **Review changes**: Always review the changes made by the script
3. **Configure appropriately**: Adjust the configuration file for your project's needs
4. **Team coordination**: Ensure all team members have the pre-commit hook installed

## Contributing

When contributing to this project:
1. The pre-commit hook will automatically clean your code
2. Ensure your IDE is configured to remove trailing whitespace
3. Follow the project's coding standards

## License

This whitespace removal system is part of the project and follows the same license terms.
