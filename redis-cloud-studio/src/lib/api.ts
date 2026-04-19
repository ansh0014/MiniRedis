const API_URL = import.meta.env.VITE_API_URL || "http://localhost:8080";

async function gatewayFetch<T>(
  endpoint: string,
  options: RequestInit = {}
): Promise<T> {
  const response = await fetch(`${API_URL}${endpoint}`, {
    ...options,
    credentials: "include",
    headers: {
      "Content-Type": "application/json",
      ...options.headers,
    },
  });

  if (!response.ok) {
    const error = await response.json().catch(() => ({ error: response.statusText }));
    throw new Error(error.error || `API Error: ${response.status}`);
  }

  const contentType = response.headers.get("content-type");
  if (contentType && contentType.includes("application/json")) {
    return response.json();
  }

  const text = await response.text();
  return text as T;
}

export interface User {
  uid: string;
  email: string;
  name?: string;
  picture?: string;
}

export interface Tenant {
  tenant_id?: string;
  id?: string;
  name: string;
  port?: number;
  node_port?: number;
  status?: string;
  tenant?: {
    id?: string;
    tenant_id?: string;
    port?: number;
    node_port?: number;
  };
}

export interface ApiKey {
  api_key: string;
  tenant_id: string;
}

export interface RedisNode {
  tenant_id: string;
  status: string;
  port: number;
  created_at?: string;
  memory_used?: number;
  key_count?: number;
}

export interface CommandResult {
  result: string;
  error?: string;
}

export const api = {
  me: () => gatewayFetch<User>("/auth/me"),
  logout: () => gatewayFetch<void>("/auth/logout", { method: "POST" }),

  createTenant: (name: string, memoryMb: number = 40) =>
    gatewayFetch<Tenant>("/api/tenants", {
      method: "POST",
      body: JSON.stringify({
        name,
        memory_limit_mb: memoryMb,
      }),
    }),

  getTenant: (id: string) => gatewayFetch<Tenant>(`/api/tenants/${id}`),

  deleteTenant: (tenantId: string) =>
    gatewayFetch<{ success: boolean }>(`/api/tenants/${tenantId}`, {
      method: "DELETE",
    }),

  createApiKey: (tenantId: string) =>
    gatewayFetch<ApiKey>("/api/apikeys", {
      method: "POST",
      body: JSON.stringify({ tenant_id: tenantId }),
    }),

  verifyApiKey: (key: string) =>
    gatewayFetch<{ tenant_id: string }>(`/api/verify?key=${encodeURIComponent(key)}`),

  revokeApiKey: (key: string) =>
    gatewayFetch<{ status: string }>(`/api/apikeys/${key}`, { method: "DELETE" }),

  listNodes: async (): Promise<RedisNode[]> => {
    const data = await gatewayFetch<any>("/api/nodes/list");
    if (Array.isArray(data)) return data;
    if (Array.isArray(data?.nodes)) return data.nodes;
    if (Array.isArray(data?.data)) return data.data;
    return [];
  },

  startNode: (tenantId: string, port: number) =>
    gatewayFetch<{ success: boolean }>("/api/nodes/start", {
      method: "POST",
      body: JSON.stringify({ tenant_id: tenantId, port }),
    }),

  stopNode: (tenantId: string) =>
    gatewayFetch<{ success: boolean }>("/api/nodes/stop", {
      method: "POST",
      body: JSON.stringify({ tenant_id: tenantId }),
    }),

  executeCommand: (tenantId: string, command: string) =>
    gatewayFetch<string>("/api/nodes/execute", {
      method: "POST",
      body: JSON.stringify({ tenant_id: tenantId, command }),
    }).then((result) => ({ result: result as unknown as string })),
};

export default api;