python3 -c '
import glob, json, os
files = glob.glob("app/**/*.c", recursive=True) + glob.glob("lib3d/**/*.c", recursive=True)
db = [{"directory": os.getcwd(), "file": f, "arguments": ["clang", "-std=c23", "-Ilib3d", "-c", f]} for f in files]
json.dump(db, open("compile_commands.json", "w"), indent=2)
'
