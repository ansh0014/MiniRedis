-- Drop if exists and recreate
DROP TABLE IF EXISTS rate_limits CASCADE;

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


INSERT INTO rate_limits (tenant_id, max_requests_per_second, max_requests_per_minute, max_requests_per_hour, max_requests_per_day)
SELECT id, 100, 1000, 10000, 100000 FROM tenants
ON CONFLICT (tenant_id) DO NOTHING;