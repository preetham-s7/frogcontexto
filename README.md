# FrogContexto

FrogContexto is a C++20 tool and Codex skill for preserving important task state around context compaction.

This first version is intentionally local and self-contained. It stores a structured Markdown checkpoint, keeps a timestamped history, and compares a saved checkpoint with a current context to identify confirmed, missing, new, and conflicting information.

## Build with g++

```powershell
g++ -std=c++20 -Wall -Wextra -pedantic -Iinclude src/context.cpp src/openai_client.cpp src/agent_runtime.cpp src/main.cpp -lwinhttp -o frogcontexto.exe
g++ -std=c++20 -Wall -Wextra -pedantic -Iinclude src/context.cpp tests/test_context.cpp -o frogcontexto_tests.exe
g++ -std=c++20 -Wall -Wextra -pedantic -Iinclude src/openai_client.cpp tests/test_json.cpp -lwinhttp -o frogcontexto_json_tests.exe
```

## Run

```powershell
.\frogcontexto.exe checkpoint . examples\context.md
.\frogcontexto.exe reconcile examples\context.md examples\current-context.md
.\frogcontexto.exe inspect .frogcontexto\context.md
```

The checkpoint command writes `.frogcontexto/context.md` and a timestamped copy under `.frogcontexto/history/`.

## API-backed runtime

The runtime now includes a Windows WinHTTP adapter for the OpenAI Responses API:

```powershell
$env:OPENAI_API_KEY = "your-key"
$env:FROGCONTEXTO_COMPACT_THRESHOLD = "200000"
.\frogcontexto.exe run .
```

Before sending a request, it estimates context size and saves a checkpoint at 80% of the threshold. When the API returns a compaction item, FrogContexto injects the saved checkpoint into the next conversation turn so the agent can reconcile and continue.

## Current scope

- Local checkpoint storage
- Markdown parsing and rendering
- Reconciliation of saved and current context
- CMake configuration
- Windows WinHTTP API adapter
- Responses API server-side compaction configuration
- Automatic checkpoint and recovery-message lifecycle
- Reusable `skills/frogcontexto/SKILL.md` definition

The API-backed runtime requires a valid `OPENAI_API_KEY`; the local checkpoint and reconciliation commands work without one.

## Simulation mode

To test the entire memory workflow without any API key or network connection:

```powershell
.\frogcontexto.exe simulate .
```

Enter messages normally. The simulator returns fake assistant responses and triggers a fake compaction every third message. FrogContexto then saves, prunes, and restores the checkpoint exactly as it would in the API-backed runtime.

## Install the Codex skill

After downloading or cloning this repository, install the skill for your user account:

```powershell
python scripts\install-skill.py
```

The skill is then available as `$frogcontexto` in Codex. The skill folder is also included at `skills/frogcontexto/` for manual installation or packaging.

The C++ executable remains optional for the skill itself. The skill can guide checkpoint and recovery workflows in a project; the executable provides local storage, simulation, and API-backed runtime behavior.

## GitHub

This repository includes a MIT license and a GitHub Actions workflow that builds and tests the project on Windows. Anyone can clone it, install the Codex skill, and run the simulator without an API key.
