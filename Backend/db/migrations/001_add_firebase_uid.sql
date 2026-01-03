
ALTER TABLE tenants ADD COLUMN IF NOT EXISTS firebase_uid VARCHAR(128);


CREATE INDEX IF NOT EXISTS idx_tenants_firebase_uid ON tenants(firebase_uid);


UPDATE tenants SET firebase_uid = 'test-user-123' WHERE firebase_uid IS NULL;

ALTER TABLE tenants ALTER COLUMN firebase_uid SET NOT NULL;