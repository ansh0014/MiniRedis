-- Enable UUID extension
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- Drop existing tables if they exist
DROP TABLE IF EXISTS api_keys CASCADE;
DROP TABLE IF EXISTS tenants CASCADE;

-- Create tenants table
CREATE TABLE tenants (
  id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
  name TEXT NOT NULL,
  node_port INT NOT NULL,
  memory_limit_mb INT NOT NULL DEFAULT 40,
  created_at TIMESTAMP WITH TIME ZONE DEFAULT now()
);

-- Create api_keys table
CREATE TABLE api_keys (
  key TEXT PRIMARY KEY,
  tenant_id UUID NOT NULL REFERENCES tenants(id) ON DELETE CASCADE,
  description TEXT,
  created_at TIMESTAMP WITH TIME ZONE DEFAULT now()
);

-- Create index for faster lookups
CREATE INDEX idx_api_keys_tenant ON api_keys(tenant_id);

-- Insert sample tenants for testing
INSERT INTO tenants (id, name, node_port, memory_limit_mb) VALUES 
  (uuid_generate_v4(), 'Acme Corporation', 6379, 40),
  (uuid_generate_v4(), 'Tech Startup Inc', 6380, 50),
  (uuid_generate_v4(), 'Global Enterprise', 6381, 100);

-- Insert sample API keys (you can generate real ones via the backend API)
INSERT INTO api_keys (key, tenant_id, description) 
SELECT 
  'test-key-' || substr(md5(random()::text), 1, 16),
  id,
  'Auto-generated test key for ' || name
FROM tenants;

-- Display created data
SELECT 
  t.id as tenant_id,
  t.name as tenant_name,
  t.node_port,
  t.memory_limit_mb,
  a.key as api_key,
  a.description
FROM tenants t
LEFT JOIN api_keys a ON t.id = a.tenant_id
ORDER BY t.name;