import { McpServer } from "@modelcontextprotocol/sdk/server/mcp.js";
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import { z } from "zod";

const OWNER = process.env.GITHUB_OWNER ?? "iikawalucas-13";
const REPO = process.env.GITHUB_REPO ?? "SwitchFileHub-";
const TOKEN = process.env.GITHUB_TOKEN;

const server = new McpServer({
  name: "switchfilehub",
  version: "0.1.0",
});

const allowedPaths = new Set([
  "README.md",
  "LIMITACOES.md",
  "FUNCIONALIDADES.md",
  "switch-homebrew/README.md",
  ".github/workflows/build.yml",
]);

async function github(path: string): Promise<unknown> {
  const response = await fetch(`https://api.github.com/repos/${OWNER}/${REPO}${path}`, {
    headers: {
      Accept: "application/vnd.github+json",
      "X-GitHub-Api-Version": "2022-11-28",
      "User-Agent": "SwitchFileHub-MCP/0.1.0",
      ...(TOKEN ? { Authorization: `Bearer ${TOKEN}` } : {}),
    },
    signal: AbortSignal.timeout(15000),
  });

  if (!response.ok) {
    const body = await response.text();
    throw new Error(`GitHub API respondeu HTTP ${response.status}: ${body.slice(0, 400)}`);
  }
  return response.json();
}

function result(value: unknown) {
  return {
    content: [{ type: "text" as const, text: JSON.stringify(value, null, 2) }],
  };
}

server.tool(
  "get_repository_status",
  "Consulta informações públicas e o estado básico do repositório Switch File Hub.",
  {},
  async () => {
    const [repo, branches, pulls] = await Promise.all([
      github(""),
      github("/branches?per_page=30"),
      github("/pulls?state=open&per_page=30"),
    ]);
    const r = repo as Record<string, unknown>;
    return result({
      name: r.full_name,
      description: r.description,
      default_branch: r.default_branch,
      html_url: r.html_url,
      updated_at: r.updated_at,
      branches: (branches as Array<Record<string, unknown>>).map((b) => b.name),
      open_pull_requests: (pulls as Array<Record<string, unknown>>).map((p) => ({
        number: p.number,
        title: p.title,
        state: p.state,
        html_url: p.html_url,
        merged_at: p.merged_at,
      })),
    });
  },
);

server.tool(
  "list_recent_workflow_runs",
  "Lista execuções recentes do GitHub Actions para acompanhar compilações.",
  {
    workflow_file: z.string().optional().describe("Arquivo do workflow, por exemplo build.yml. Se omitido, lista execuções recentes do repositório."),
    per_page: z.number().int().min(1).max(20).default(10),
  },
  async ({ workflow_file, per_page }) => {
    const endpoint = workflow_file
      ? `/actions/workflows/${encodeURIComponent(workflow_file)}/runs?per_page=${per_page}`
      : `/actions/runs?per_page=${per_page}`;
    const data = await github(endpoint) as Record<string, unknown>;
    const runs = (data.workflow_runs ?? []) as Array<Record<string, unknown>>;
    return result(runs.map((run) => ({
      id: run.id,
      name: run.name,
      event: run.event,
      status: run.status,
      conclusion: run.conclusion,
      head_branch: run.head_branch,
      head_sha: run.head_sha,
      html_url: run.html_url,
      created_at: run.created_at,
      updated_at: run.updated_at,
    })));
  },
);

server.tool(
  "get_workflow_run_details",
  "Consulta detalhes de uma execução e seus jobs no GitHub Actions.",
  { run_id: z.number().int().positive() },
  async ({ run_id }) => {
    const [run, jobs] = await Promise.all([
      github(`/actions/runs/${run_id}`),
      github(`/actions/runs/${run_id}/jobs?per_page=50`),
    ]);
    const r = run as Record<string, unknown>;
    const j = jobs as Record<string, unknown>;
    return result({
      id: r.id,
      name: r.name,
      status: r.status,
      conclusion: r.conclusion,
      html_url: r.html_url,
      head_branch: r.head_branch,
      head_sha: r.head_sha,
      jobs: ((j.jobs ?? []) as Array<Record<string, unknown>>).map((job) => ({
        name: job.name,
        status: job.status,
        conclusion: job.conclusion,
        html_url: job.html_url,
        steps: job.steps,
      })),
    });
  },
);

server.tool(
  "read_project_file",
  "Lê apenas arquivos de documentação e configuração previamente permitidos. Não lê segredos.",
  { path: z.string().describe("Caminho permitido no repositório.") },
  async ({ path }) => {
    if (!allowedPaths.has(path)) {
      throw new Error("Caminho não permitido. Este servidor só lê arquivos públicos de documentação e configuração listados na documentação MCP.");
    }
    const data = await github(`/contents/${path.split("/").map(encodeURIComponent).join("/")}`) as Record<string, unknown>;
    if (data.encoding !== "base64" || typeof data.content !== "string") {
      throw new Error("O GitHub não retornou o conteúdo em Base64.");
    }
    const content = Buffer.from(data.content.replace(/\n/g, ""), "base64").toString("utf8");
    return result({ path, content, html_url: data.html_url });
  },
);

async function main() {
  const transport = new StdioServerTransport();
  await server.connect(transport);
}

main().catch((error: unknown) => {
  // stdout é reservado ao protocolo MCP; erros vão para stderr.
  console.error(error instanceof Error ? error.message : String(error));
  process.exitCode = 1;
});
