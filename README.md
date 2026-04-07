# BoostChain

**C++ Agent Framework for LLM Applications**

> **This project is discontinued and will no longer be maintained.**

## Why

When this project was started, there were few C++ options for building LLM-powered agents. Since then, the ecosystem has matured rapidly, and several alternatives now offer better features, active maintenance, and community support:

| Project | Description |
|---------|-------------|
| [agent.cpp](https://github.com/mozilla-ai/agent.cpp) | Mozilla AI's local agent library with MCP support |
| [agent-sdk-cpp](https://github.com/abdomody35/agent-sdk-cpp) | Header-only ReAct agent library with parallel tool calling |
| [llm-cpp](https://github.com/Mattbusel/llm-cpp) | 26 single-header libraries covering agents, RAG, streaming, retry, and more |

BoostChain has no unique advantage over these projects. Key shortcomings:

- No streaming support (placeholder only)
- No Claude tool_use support
- No connection reuse in HTTP client
- No RAG pipeline
- No MCP protocol support

Continuing development would mean reinventing what already exists in a more complete form elsewhere.

## What's Here

The repository is kept for reference. The code is functional for basic use cases (single-turn chat, simple chains, agent loop with calculator tool), but is not recommended for production.

## License

MIT
