"""열려 있는 언리얼 에디터에 Python을 보내 실행하는 클라이언트 (9-3, 배포판 작업 도구).

에디터 쪽: PythonScriptPlugin의 Remote Execution을 켠다
  (Saved/Config/WindowsEditor/Engine.ini, [/Script/PythonScriptPlugin.PythonScriptPluginSettings] bRemoteExecution=True).
통신은 로컬(127.0.0.1)만 쓴다. 엔진의 remote_execution.py를 그대로 쓴다.

사용법:
  python Tools/ue_exec.py -c "import unreal; print(unreal.SystemLibrary.get_engine_version())"
  python Tools/ue_exec.py script.py
  python Tools/ue_exec.py --console "FirstAction.DumpBlueprintGraph /Game/ThirdParty/Release/Greystone/ABP_Greystone"
"""

import argparse
import os
import sys
import time

ENGINE_PYTHON = os.environ.get(
    "UE_PYTHON_DIR",
    r"F:/Epic Games/Games/UE_5.7/Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python",
)
sys.path.insert(0, ENGINE_PYTHON)

import remote_execution  # noqa: E402  (엔진 제공 모듈)

PROJECT_NAME = "FirstAction"


def find_node(remote, timeout):
    deadline = time.time() + timeout
    while time.time() < deadline:
        for node in remote.remote_nodes:
            if node.get("project_name") == PROJECT_NAME:
                return node
        time.sleep(0.2)
    return None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("file", nargs="?", help="실행할 Python 파일")
    parser.add_argument("-c", dest="code", help="실행할 Python 코드")
    parser.add_argument("--console", help="실행할 에디터 콘솔 명령")
    parser.add_argument("--timeout", type=float, default=5.0, help="에디터를 찾는 시간(초)")
    args = parser.parse_args()

    if args.console:
        code = "import unreal\nunreal.SystemLibrary.execute_console_command(None, %r)\n" % args.console
    elif args.code:
        code = args.code
    elif args.file:
        with open(args.file, encoding="utf-8") as f:
            code = f.read()
    else:
        parser.error("파일, -c, --console 중 하나가 필요합니다")

    remote = remote_execution.RemoteExecution()
    remote.start()
    try:
        node = find_node(remote, args.timeout)
        if not node:
            print("에디터를 찾지 못했습니다. 에디터가 열려 있고 Remote Execution이 켜져 있는지 확인하세요.", file=sys.stderr)
            return 2
        remote.open_command_connection(node["node_id"])
        result = remote.run_command(code, unattended=True, exec_mode=remote_execution.MODE_EXEC_FILE)
        for entry in result.get("output", []):
            text = entry.get("output", "")
            print(text, end="" if text.endswith("\n") else "\n")
        if not result.get("success", False):
            print(result.get("result", ""), file=sys.stderr)
            return 1
        return 0
    finally:
        remote.stop()


if __name__ == "__main__":
    sys.exit(main())
