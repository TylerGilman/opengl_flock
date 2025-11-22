# WASM Flocking Animation - Integration Guide

A guide for integrating the WebAssembly flocking animation into any web project and serving it efficiently via Cloudflare CDN.

## Table of Contents
- [Quick Start](#quick-start)
- [Files You Need](#files-you-need)
- [Integration Examples](#integration-examples)
- [Cloudflare CDN Setup](#cloudflare-cdn-setup)
- [Verification & Testing](#verification--testing)
- [Troubleshooting](#troubleshooting)

---

## Quick Start

**Total file size:** ~71KB (46KB JS + 25KB WASM)

### 1. Copy Files to Your Project

```bash
# Copy the 2 WASM files to your project
cp flock_wasm.js flock_wasm.wasm /path/to/your-project/public/particles/
```

### 2. Add to HTML

```html
<canvas id="canvas"></canvas>
<script src="/particles/flock_wasm.js"></script>
```

That's it! The animation will start automatically.

---

## Files You Need

Only **2 files** are required:

```
flock_wasm.js      # Emscripten glue code (~46KB)
flock_wasm.wasm    # Compiled WebAssembly binary (~25KB)
```

**Do NOT commit to git:**
- Build artifacts (`*.o` files)
- Source code (unless you want to rebuild)
- `emsdk/` folder

**Safe to commit to git:**
- The 2 WASM files (they're small and pre-built)

---

## Integration Examples

### Static HTML Site

```html
<!DOCTYPE html>
<html>
<head>
    <style>
        body { margin: 0; overflow: hidden; }
        #canvas {
            position: fixed;
            top: 0;
            left: 0;
            width: 100vw;
            height: 100vh;
            z-index: 0;
        }
        .content {
            position: relative;
            z-index: 1;
            padding: 2rem;
        }
    </style>
</head>
<body>
    <!-- Canvas for particles -->
    <canvas id="canvas"></canvas>

    <!-- Your content -->
    <div class="content">
        <h1>Your Content Here</h1>
    </div>

    <!-- Canvas resizing script -->
    <script>
        const canvas = document.getElementById('canvas');

        function resizeCanvas() {
            const dpr = window.devicePixelRatio || 1;
            canvas.width = window.innerWidth * dpr;
            canvas.height = window.innerHeight * dpr;
        }

        resizeCanvas();
        window.addEventListener('resize', resizeCanvas);

        // Optional: WASM ready callback
        window.Module = {
            onRuntimeInitialized: function() {
                console.log('Flocking animation loaded!');
            }
        };
    </script>

    <!-- Load WASM -->
    <script src="/particles/flock_wasm.js"></script>
</body>
</html>
```

### React/Next.js

```javascript
// components/FlockingBackground.jsx
import { useEffect } from 'react';

export default function FlockingBackground() {
  useEffect(() => {
    // Resize canvas
    const canvas = document.getElementById('canvas');
    const resizeCanvas = () => {
      const dpr = window.devicePixelRatio || 1;
      canvas.width = window.innerWidth * dpr;
      canvas.height = window.innerHeight * dpr;
    };

    resizeCanvas();
    window.addEventListener('resize', resizeCanvas);

    // Load WASM
    const script = document.createElement('script');
    script.src = '/particles/flock_wasm.js';
    script.async = true;
    document.body.appendChild(script);

    return () => {
      window.removeEventListener('resize', resizeCanvas);
      document.body.removeChild(script);
    };
  }, []);

  return (
    <canvas
      id="canvas"
      style={{
        position: 'fixed',
        top: 0,
        left: 0,
        width: '100vw',
        height: '100vh',
        zIndex: 0
      }}
    />
  );
}
```

**Usage:**
```javascript
// pages/index.js
import FlockingBackground from '@/components/FlockingBackground';

export default function Home() {
  return (
    <>
      <FlockingBackground />
      <main style={{ position: 'relative', zIndex: 1 }}>
        <h1>Your Content</h1>
      </main>
    </>
  );
}
```

**File placement for Next.js:**
```
your-nextjs-project/
├── public/
│   └── particles/
│       ├── flock_wasm.js
│       └── flock_wasm.wasm
```

### Vue.js

```vue
<!-- components/FlockingBackground.vue -->
<template>
  <canvas id="canvas" ref="canvas"></canvas>
</template>

<script>
export default {
  mounted() {
    this.resizeCanvas();
    window.addEventListener('resize', this.resizeCanvas);

    // Load WASM
    const script = document.createElement('script');
    script.src = '/particles/flock_wasm.js';
    document.body.appendChild(script);
  },
  beforeUnmount() {
    window.removeEventListener('resize', this.resizeCanvas);
  },
  methods: {
    resizeCanvas() {
      const canvas = this.$refs.canvas;
      const dpr = window.devicePixelRatio || 1;
      canvas.width = window.innerWidth * dpr;
      canvas.height = window.innerHeight * dpr;
    }
  }
}
</script>

<style scoped>
canvas {
  position: fixed;
  top: 0;
  left: 0;
  width: 100vw;
  height: 100vh;
  z-index: 0;
}
</style>
```

---

## Cloudflare CDN Setup

Cloudflare will route traffic through their network automatically, but **will NOT cache WASM files** by default. You need to configure caching.

### Prerequisites

1. Domain pointed to Cloudflare (nameservers updated)
2. Files uploaded to your server
3. Server configured with correct MIME types

### Step 1: Upload Files to Your Server

```bash
# SSH into your VPS
ssh user@your-server

# Create directory
mkdir -p /var/www/html/particles/

# Upload files (from your local machine)
scp flock_wasm.js flock_wasm.wasm user@your-server:/var/www/html/particles/
```

### Step 2: Configure Nginx MIME Types

SSH into your server and edit nginx config:

```bash
sudo nano /etc/nginx/sites-available/default
```

Add this to your `server` block:

```nginx
server {
    listen 80;
    server_name yourdomain.com www.yourdomain.com;

    # Particles/WASM location
    location /particles/ {
        alias /var/www/html/particles/;

        # Set correct MIME type for WASM
        types {
            application/wasm wasm;
            application/javascript js;
        }

        # Cache headers (Cloudflare will respect these)
        add_header Cache-Control "public, max-age=31536000, immutable";
        add_header Access-Control-Allow-Origin "*";

        # Enable compression
        gzip on;
        gzip_types application/wasm application/javascript;
    }
}
```

Test and reload:

```bash
sudo nginx -t
sudo systemctl reload nginx
```

### Step 3: Configure Cloudflare Caching

#### Option A: Page Rules (Simple - Free Tier Includes 3 Rules)

1. Login to Cloudflare Dashboard
2. Select your domain
3. Go to **Rules** → **Page Rules**
4. Click **Create Page Rule**
5. Configure:
   - **URL Pattern:** `*yourdomain.com/particles/*`
   - **Settings:**
     - Cache Level: **Cache Everything**
     - Edge Cache TTL: **1 month**
     - Browser Cache TTL: **1 year**
6. Click **Save and Deploy**

#### Option B: Cache Rules (Recommended - More Flexible)

1. Cloudflare Dashboard → **Caching** → **Cache Rules**
2. Click **Create Rule**
3. Configure:
   - **Rule name:** Cache WASM particles
   - **When incoming requests match:**
     - Field: `URI Path`
     - Operator: `starts with`
     - Value: `/particles/`
   - **Then:**
     - **Eligible for cache:** Yes
     - **Edge TTL:** 1 month
     - **Browser TTL:** 1 year
4. Click **Deploy**

### Step 4: Enable SSL/TLS

1. Go to **SSL/TLS** → **Overview**
2. Set encryption mode to **Full** or **Full (strict)**
3. If you don't have SSL on your server:

```bash
# Install Let's Encrypt (free SSL)
sudo apt update
sudo apt install certbot python3-certbot-nginx
sudo certbot --nginx -d yourdomain.com -d www.yourdomain.com
```

---

## Verification & Testing

### Test 1: MIME Type Check

```bash
curl -I https://yourdomain.com/particles/flock_wasm.wasm

# Should show:
# Content-Type: application/wasm ✅
```

### Test 2: Cloudflare Cache Status

```bash
# First request (caching)
curl -I https://yourdomain.com/particles/flock_wasm.wasm | grep cf-cache

# Should show:
# cf-cache-status: MISS (first time)
# cf-ray: xxxxx-XXX (proves it's going through Cloudflare)

# Second request (should be cached)
curl -I https://yourdomain.com/particles/flock_wasm.wasm | grep cf-cache

# Should show:
# cf-cache-status: HIT ✅
```

### Test 3: Browser DevTools

1. Open your site in browser
2. Open DevTools (F12) → **Network** tab
3. Refresh page
4. Find `flock_wasm.wasm` in the network list
5. Check **Headers** tab:
   - `cf-cache-status: HIT` ✅
   - `content-type: application/wasm` ✅

### Test 4: Global Performance

Use https://tools.keycdn.com/performance

Enter: `https://yourdomain.com/particles/flock_wasm.wasm`

Should see fast load times from multiple global locations.

---

## Troubleshooting

### Issue: `cf-cache-status: BYPASS`

**Problem:** Cloudflare is not caching the files

**Solution:**
1. Check your Page Rule or Cache Rule URL pattern matches exactly
2. Try pattern: `*yourdomain.com/particles/*` with asterisks
3. Set Cache Level to "Cache Everything"
4. Wait 5 minutes for rule to propagate

### Issue: `cf-cache-status: DYNAMIC`

**Problem:** Cloudflare thinks the file is dynamic content

**Solution:**
1. Make sure you set "Cache Level: Cache Everything" in your rule
2. Check that the rule is deployed and active

### Issue: WASM Shows as `text/plain`

**Problem:** Nginx MIME type not configured

**Solution:**
```bash
# Check nginx config has:
types {
    application/wasm wasm;
}

# Reload nginx
sudo systemctl reload nginx
```

### Issue: "Too Many Redirects"

**Problem:** SSL/TLS misconfiguration

**Solution:**
1. Go to Cloudflare → **SSL/TLS** → **Overview**
2. Change to **Full** (not Flexible)
3. Ensure your server has an SSL certificate

### Issue: Files Not Loading (404)

**Problem:** Path mismatch

**Solution:**
```bash
# Check files exist
ls -la /var/www/html/particles/

# Check nginx config path
# location /particles/ {
#     alias /var/www/html/particles/;  ← Check this path
# }

# Test direct server access
curl http://your-server-ip/particles/flock_wasm.wasm
```

### Issue: CORS Error

**Problem:** Files loaded from different origin

**Solution:**
Add to nginx config:
```nginx
add_header Access-Control-Allow-Origin "*";
```

---

## Updating WASM Files

When you rebuild and want to deploy new versions:

### 1. Upload New Files

```bash
scp flock_wasm.js flock_wasm.wasm user@server:/var/www/html/particles/
```

### 2. Purge Cloudflare Cache

**Option A: Dashboard**
1. Cloudflare → **Caching** → **Configuration**
2. Click **Purge Cache** → **Custom Purge**
3. Enter URLs:
   ```
   https://yourdomain.com/particles/flock_wasm.js
   https://yourdomain.com/particles/flock_wasm.wasm
   ```
4. Click **Purge**

**Option B: API (Faster)**
```bash
curl -X POST "https://api.cloudflare.com/client/v4/zones/YOUR_ZONE_ID/purge_cache" \
  -H "Authorization: Bearer YOUR_API_TOKEN" \
  -H "Content-Type: application/json" \
  --data '{"files":["https://yourdomain.com/particles/flock_wasm.js","https://yourdomain.com/particles/flock_wasm.wasm"]}'
```

---

## Performance Tips

### 1. Lazy Load on Scroll

Only load the animation when visible:

```javascript
const observer = new IntersectionObserver((entries) => {
  entries.forEach(entry => {
    if (entry.isIntersecting) {
      const script = document.createElement('script');
      script.src = '/particles/flock_wasm.js';
      document.body.appendChild(script);
      observer.disconnect();
    }
  });
});

observer.observe(document.querySelector('#canvas'));
```

### 2. Pause When Tab Hidden

Save CPU/battery when tab is not visible:

```javascript
document.addEventListener('visibilitychange', () => {
  if (document.hidden && typeof Module !== 'undefined') {
    if (Module.pauseMainLoop) Module.pauseMainLoop();
  } else {
    if (Module.resumeMainLoop) Module.resumeMainLoop();
  }
});
```

### 3. Reduce Particles on Mobile

The WASM is configured for 500 particles. For mobile, you might want fewer.

Rebuild with different particle count:
```c
// Edit main_wasm.c line 22
#define NUM_PARTICLES 300  // Reduced for mobile
```

Then rebuild:
```bash
make wasm
```

---

## Advanced: Multiple Instances

To run multiple animations on one page:

1. Each needs a unique canvas ID
2. Modify `main_wasm.c` to accept canvas ID as parameter
3. Rebuild and create multiple WASM files

This is complex - usually one background animation is sufficient.

---

## Support

For issues with:
- **WASM compilation:** See `README.md` in source repo
- **Cloudflare setup:** https://developers.cloudflare.com/cache/
- **Performance optimization:** See `PERFORMANCE.md`

---

## Summary Checklist

- [ ] Copy `flock_wasm.js` and `flock_wasm.wasm` to your project
- [ ] Add `<canvas id="canvas"></canvas>` to HTML
- [ ] Load script: `<script src="/particles/flock_wasm.js"></script>`
- [ ] Upload files to your server
- [ ] Configure nginx MIME types
- [ ] Add Cloudflare Page Rule or Cache Rule
- [ ] Test: `cf-cache-status: HIT` in headers
- [ ] Check browser DevTools for proper loading

**Total setup time:** 15-20 minutes

Your WASM flocking animation is now served globally via Cloudflare CDN! 🚀
