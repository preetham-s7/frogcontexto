---
name: frogcontexto
description: Preserve important task state before context compaction and reconcile it afterward using the FrogContexto C++ runtime.
---

# FrogContexto

Use this skill for long-running tasks where losing decisions, pending work, or user preferences after context compaction would be harmful.

## Workflow

1. Before a long operation or when the context is approaching its configured threshold, run:

   ```powershell
   .\frogcontexto.exe checkpoint . path\to\context.md
   ```

2. After compaction, compare the checkpoint with the newest context:

   ```powershell
   .\frogcontexto.exe reconcile .frogcontexto\context.md path\to\current-context.md
   ```

3. Treat conflicts as unresolved until the newest user instruction or current file state establishes the truth.
4. Restore missing decisions and pending work before continuing.
5. Never treat a checkpoint as permission to ignore newer user instructions.

## Runtime mode

For an API-backed agent, run:

```powershell
$env:OPENAI_API_KEY = "..."
.\frogcontexto.exe run .
```

The runtime sends Responses API requests with server-side compaction enabled, checkpoints when the estimated context reaches 80% of the configured threshold, and injects the saved checkpoint into the next context after a compaction item is returned.

Optional environment variables:

- `FROGCONTEXTO_MODEL`
- `FROGCONTEXTO_COMPACT_THRESHOLD`

The checkpoint is local and may contain conversation data. Keep `.frogcontexto/` out of source control and do not commit secrets.
