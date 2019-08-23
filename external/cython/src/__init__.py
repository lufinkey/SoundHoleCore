
# an exception just to confirm that the .so file is loaded instead of the .py file
raise ImportError("__init__.py loaded when __init__.so should have been loaded")
