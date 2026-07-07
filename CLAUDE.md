## graphify

This project has a knowledge graph at graphify-out/ with god nodes, community structure, and cross-file relationships.

Rules:
- For codebase questions, first run `graphify query "<question>"` when graphify-out/graph.json exists. Use `graphify path "<A>" "<B>"` for relationships and `graphify explain "<concept>"` for focused concepts. These return a scoped subgraph, usually much smaller than GRAPH_REPORT.md or raw grep output.
- If graphify-out/wiki/index.md exists, use it for broad navigation instead of raw source browsing.
- Read graphify-out/GRAPH_REPORT.md only for broad architecture review or when query/path/explain do not surface enough context.
- After modifying code, run `graphify update .` to keep the graph current (AST-only, no API cost).

## Building (Windows)

MSBuild isn't on PATH; locate it via vswhere, then build the VS2019 project directly:

```powershell
$msbuild = & "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe"
& $msbuild "D:\GitHub\exult\msvcstuff\vs2019\Exult.vcxproj" /p:Configuration=Release /p:Platform=x64
```

- Default target links too — it updates the real `Exult.exe` at the repo root (`OutDir` is set to repo root). That's the one to check.
- `/t:ClCompile` only compiles `.obj` files and does NOT relink — the root `Exult.exe` stays stale. Only use it for a fast syntax/type check, never as proof the build actually updated, and say so explicitly if that's all you ran.
- To confirm the exe was actually rebuilt, check its timestamp/size at repo-root `Exult.exe`, not just that MSBuild reported success.
- Config/platform must match an existing output dir under `msvcstuff/vs2019/{Debug,Release}/Exult/` — Release|x64 is what's normally used here.
- Incremental: unchanged .cc files are skipped. To confirm a specific file actually recompiled, check its .obj timestamp in `msvcstuff/vs2019/{Debug,Release}/Exult/<file>.obj` against the source file's mtime.
