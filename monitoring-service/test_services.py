import requests
import time

SERVICES = {
    "Auth Service": "http://localhost:8000/health",
    "API Gateway": "http://localhost:8080/health",
    "Backend": "http://localhost:5500/health",
    "Node Manager": "http://localhost:7000/node/list",
    "Monitoring Service": "http://localhost:9000/health",
}

def test_service(name, url):
    try:
        response = requests.get(url, timeout=3)
        if response.status_code == 200:
            print(f" {name:20} - CONNECTED")
            return True
        else:
            print(f" {name:20} - HTTP {response.status_code}")
            return False
    except requests.exceptions.ConnectionError:
        print(f" {name:20} - NOT RUNNING")
        return False
    except Exception as e:
        print(f" {name:20} - ERROR: {e}")
        return False

def main():
    print("\n" + "="*60)
    print(" MiniRedis Services Health Check")
    print("="*60 + "\n")
    
    results = {}
    for name, url in SERVICES.items():
        results[name] = test_service(name, url)
        time.sleep(0.2)
    
    print("\n" + "="*60)
    total = len(results)
    running = sum(results.values())
    print(f" Status: {running}/{total} services running")
    
    if running == total:
        print(" ALL SERVICES CONNECTED!")
    else:
        print("  Some services are not running:")
        for name, status in results.items():
            if not status:
                print(f"   - {name}")
    
    print("="*60 + "\n")

if __name__ == "__main__":
    main()