# Endstone Python Example Plugin

Welcome to the example Python plugin for Endstone servers.

## Prerequisites

- Python 3.9 or higher.
- Endstone installed and set up in your Python environment.

## Structure Overview

```
python-example-plugin/ 
├── src/                         # Main source directory 
│   └── endstone_example/        # Directory for the plugin package 
│       ├── __init__.py          # Initializer for the package, importing ExamplePlugin class from example_plugin.py
│       ├── example_plugin.py    # Implementation of ExamplePlugin class
│       └── python_command.py    # Custom command executor for /python
├── .gitignore                   # Git ignore rules
├── LICENSE                      # License details
├── README.md                    # This file
└── pyproject.toml               # Plugin configuration file which specifies the entrypoint
```

## Getting Started

1. **Clone this Repository**

   ```bash
   git clone https://github.com/EndstoneMC/python-example-plugin.git
   ```

2. **Navigate to the Cloned Directory**

   ```bash
   cd python-example-plugin
   ```

3. **Install Your Plugin**

   When developing the plugin, you may want to install an editable package to your Python environment, this allows you
   to update the codes without having to reinstall the package everytime:
   ```bash
   pip install -e .
   ```
   **NOTE: It is strongly recommended to create a virtual environment for your Endstone server and plugins. When
   installing your plugin using `pip install`, please ensure the virtual environment is activated.**

   Ensure your plugin is loaded correctly by checking the server logs or console for the log messages.

4. **Package and Distribute Your Plugin**

   When everything is good to go, you can package your plugin into a `.whl` (Wheel) file for easier distribution:

   ```bash
   pip install pipx
   pipx run build --wheel
   ```

   This command will produce a `.whl` file in the `dist` directory. Copy the `.whl` file to the `plugins` directory
   of your Endstone server. Start the Endstone server and check the logs to ensure your plugin loads and operates
   as expected.

   To publish your plugin to a package index such as PyPI, please refer to:
    - [Using TestPyPI](https://packaging.python.org/en/latest/guides/using-testpypi/)
    - [Publishing package distribution releases using GitHub Actions CI/CD workflows](https://packaging.python.org/en/latest/guides/publishing-package-distribution-releases-using-github-actions-ci-cd-workflows/)

## Documentation

For a deeper dive into the Endstone API and its functionalities, refer to the main
Endstone [documentation](https://endstone.readthedocs.io) (WIP).

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
