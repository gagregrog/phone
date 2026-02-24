Import("env")
import os


def read_dotenv(project_dir):
    env_vars = {}
    try:
        with open(os.path.join(project_dir, ".env")) as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#") or "=" not in line:
                    continue
                key, _, value = line.partition("=")
                env_vars[key.strip()] = value.strip()
    except FileNotFoundError:
        pass
    return env_vars


env_vars = read_dotenv(env["PROJECT_DIR"])
port = env_vars.get("PORT", "")

if port:
    env["MONITOR_PORT"] = port


def inject_upload_port(source, target, env):
    env_vars = read_dotenv(env["PROJECT_DIR"])
    port = env_vars.get("PORT", "")
    if port:
        env["UPLOAD_PORT"] = port


env.AddPreAction("upload", inject_upload_port)
