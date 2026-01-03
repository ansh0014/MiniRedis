
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";


DROP TABLE IF EXISTS rate_limits CASCADE;
DROP TABLE IF EXISTS api_keys CASCADE;
DROP TABLE IF EXISTS tenants CASCADE;


CREATE TABLE tenants (
  id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
  name TEXT NOT NULL,
  firebase_uid VARCHAR(128) NOT NULL, 
  node_port INT NOT NULL UNIQUE,
  memory_limit_mb INT NOT NULL DEFAULT 40,
  status VARCHAR(50) DEFAULT 'active',
  created_at TIMESTAMP WITH TIME ZONE DEFAULT now(),
  updated_at TIMESTAMP WITH TIME ZONE DEFAULT now()
);

CREATE INDEX idx_tenants_firebase_uid ON tenants(firebase_uid);
CREATE INDEX idx_tenants_status ON tenants(status);


CREATE TABLE api_keys (
  key TEXT PRIMARY KEY,
  tenant_id UUID NOT NULL REFERENCES tenants(id) ON DELETE CASCADE,
  description TEXT,
  is_active BOOLEAN DEFAULT true,
  last_used_at TIMESTAMP WITH TIME ZONE,
  created_at TIMESTAMP WITH TIME ZONE DEFAULT now()
);

CREATE INDEX idx_api_keys_tenant ON api_keys(tenant_id);
CREATE INDEX idx_api_keys_active ON api_keys(is_active);


CREATE TABLE rate_limits (
  id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
  tenant_id UUID NOT NULL REFERENCES tenants(id) ON DELETE CASCADE,
  

  max_requests_per_second INT NOT NULL DEFAULT 100,
  max_requests_per_minute INT NOT NULL DEFAULT 1000,
  max_requests_per_hour INT NOT NULL DEFAULT 10000,
  max_requests_per_day INT NOT NULL DEFAULT 100000,
  

  requests_current_second INT DEFAULT 0,
  requests_current_minute INT DEFAULT 0,
  requests_current_hour INT DEFAULT 0,
  requests_current_day INT DEFAULT 0,
  

  second_reset_at TIMESTAMP WITH TIME ZONE DEFAULT now(),
  minute_reset_at TIMESTAMP WITH TIME ZONE DEFAULT now(),
  hour_reset_at TIMESTAMP WITH TIME ZONE DEFAULT now(),
  day_reset_at TIMESTAMP WITH TIME ZONE DEFAULT now(),
  
  created_at TIMESTAMP WITH TIME ZONE DEFAULT now(),
  updated_at TIMESTAMP WITH TIME ZONE DEFAULT now(),
  
  CONSTRAINT rate_limits_tenant_unique UNIQUE (tenant_id)
);

CREATE INDEX idx_rate_limits_tenant ON rate_limits(tenant_id);




INSERT INTO tenants (name, firebase_uid, node_port, memory_limit_mb) VALUES 
  ('Test Tenant 1', 'test-user-123', 6380, 40),
  ('Test Tenant 2', 'test-user-123', 6381, 50),
  ('Demo Tenant', 'demo-user-456', 6382, 100);


INSERT INTO api_keys (key, tenant_id, description) 
SELECT 
  'mk_test_' || substr(md5(random()::text), 1, 32),
  id,
  'Auto-generated test key for ' || name
FROM tenants;


INSERT INTO rate_limits (tenant_id)
SELECT id FROM tenants;

SELECT 
  t.id as tenant_id,
  t.name as tenant_name,
  t.firebase_uid,
  t.node_port,
  t.memory_limit_mb,
  t.status,
  a.key as api_key,
  a.description,
  r.max_requests_per_second,
  r.max_requests_per_minute
FROM tenants t
LEFT JOIN api_keys a ON t.id = a.tenant_id
LEFT JOIN rate_limits r ON t.id = r.tenant_id
ORDER BY t.created_at;