const API_URL = import.meta.env.VITE_API_URL || 'http://localhost:8080';

async function gatewayFetch<T>(
  endpoint: string,
  options: RequestInit = {}
): Promise<T> {
  const response = await fetch(`${API_URL}${endpoint}`, {
    ...options,
    credentials: 'include',
    headers: {
      'Content-Type': 'application/json',
      ...options.headers,
    },
  });

  if (!response.ok) {
    const error = await response.json().catch(() => ({ error: response.statusText }));
    throw new Error(error.error || `API Error: ${response.status}`);
  }

  const contentType = response.headers.get('content-type');
  if (contentType && contentType.includes('application/json')) {
    return response.json();
  } else {
    const text = await response.text();
    return text as T;
  }
}

export interface User {
  uid: string;
  email: string;
  name?: string;
  picture?: string;
}

export interface Tenant {
  tenant_id: string;
  name: string;
  port: number;
  status: string;
}

export interface ApiKey {
  api_key: string;
  tenant_id: string;
}

export interface RedisNode {
  tenant_id: string;
  status: string;
  port: number;
  created_at: string;
  memory_used?: number;
  key_count?: number;
}

export interface CommandResult {
  result: string;
  error?: string;
}

export const api = {
  me: () => gatewayFetch<User>('/auth/me'),
  logout: () => gatewayFetch<void>('/auth/logout', { method: 'POST' }),

  createTenant: (name: string, memoryMb: number = 40) =>
    gatewayFetch<Tenant>('/api/tenants', {
      method: 'POST',
      body: JSON.stringify({ 
        name, 
        memory_limit_mb: memoryMb
      }),
    }),

  getTenant: (id: string) => gatewayFetch<Tenant>(`/api/tenants/${id}`),

  createApiKey: (tenantId: string) =>
    gatewayFetch<ApiKey>('/api/apikeys', {
      method: 'POST',
      body: JSON.stringify({ tenant_id: tenantId }),
    }),

  verifyApiKey: (key: string) =>
    gatewayFetch<{ tenant_id: string }>(`/api/verify?key=${key}`),

  revokeApiKey: (key: string) =>
    gatewayFetch<{ status: string }>(`/api/apikeys/${key}`, { method: 'DELETE' }),

  listNodes: () => gatewayFetch<RedisNode[]>('/api/nodes/list'),

  startNode: (tenantId: string, port: number) =>
    gatewayFetch<{ success: boolean }>('/api/nodes/start', {
      method: 'POST',
      body: JSON.stringify({ tenant_id: tenantId, port }),
    }),

  stopNode: (tenantId: string) =>
    gatewayFetch<{ success: boolean }>('/api/nodes/stop', {
      method: 'POST',
      body: JSON.stringify({ tenant_id: tenantId }),
    }),

  executeCommand: (tenantId: string, command: string) =>
    gatewayFetch<string>('/api/nodes/execute', {
      method: 'POST',
      body: JSON.stringify({ tenant_id: tenantId, command }),
    }).then((result) => {
      return { result: result as unknown as string };
    }),
};

export default api;