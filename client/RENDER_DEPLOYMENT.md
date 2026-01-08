# Deploying Chess Client to Render.com

## Prerequisites
1. A GitHub account
2. A Render.com account (free tier available)
3. Your code pushed to a GitHub repository

## Step-by-Step Deployment Guide

### Step 1: Prepare Your Repository
Ensure your `client` folder contains:
- ✅ `index.html` - Your main application file
- ✅ `js/` folder - JavaScript files
- ✅ `requirements.txt` - Python dependencies (already created)
- ✅ `render.yaml` - Render configuration (already created)
- ✅ `start.sh` - Startup script (already created)

### Step 2: Push to GitHub
```bash
# If not already in a git repository
git init
git add .
git commit -m "Add Render deployment configuration"

# Push to GitHub
git remote add origin <your-github-repo-url>
git push -u origin main
```

### Step 3: Deploy on Render.com

#### Option A: Using render.yaml (Blueprint - Recommended)
1. Go to https://render.com and sign in
2. Click **"New +"** → **"Blueprint"**
3. Connect your GitHub account if not already connected
4. Select your repository
5. Render will automatically detect `render.yaml`
6. Click **"Apply"**
7. Wait for deployment to complete (2-3 minutes)

#### Option B: Manual Web Service Setup
1. Go to https://render.com and sign in
2. Click **"New +"** → **"Web Service"**
3. Connect your GitHub repository
4. Configure the service:
   - **Name**: `chess-client` (or your preferred name)
   - **Region**: Choose closest to your users
   - **Branch**: `main` (or your branch)
   - **Root Directory**: `client`
   - **Environment**: `Python 3`
   - **Build Command**: `echo "No build required"`
   - **Start Command**: `python3 -m http.server 8000`
   - **Plan**: `Free`
5. Click **"Create Web Service"**
6. Wait for deployment to complete

### Step 4: Configure Port (Important!)
Render provides a PORT environment variable. You need to modify the start command to use it:

**Updated Start Command:**
```bash
python3 -m http.server $PORT
```

Or update `render.yaml`:
```yaml
startCommand: python3 -m http.server $PORT
```

### Step 5: Access Your Application
Once deployed, Render will provide you with a URL like:
```
https://chess-client-xxxx.onrender.com
```

Your `index.html` will be accessible at this URL.

## Important Notes

### Free Tier Limitations
- ⚠️ **Cold Starts**: Free tier services spin down after 15 minutes of inactivity
- ⚠️ **Startup Time**: First request may take 30-60 seconds to wake up the service
- ⚠️ **750 hours/month**: Free tier limit

### WebSocket Connections
If your application uses WebSocket to connect to a backend server:
1. Ensure your WebSocket server is also deployed and accessible
2. Update WebSocket URLs in your JavaScript files to point to the production server
3. Check CORS settings on your backend server

### Custom Domain (Optional)
1. Go to your service **Settings**
2. Click **"Custom Domain"**
3. Add your domain and configure DNS

## Troubleshooting

### Issue: Application not loading
- Check **Logs** in Render dashboard
- Verify the start command is correct
- Ensure PORT environment variable is used

### Issue: WebSocket connection fails
- Update WebSocket URLs in [js/game.js](js/game.js), [js/lobby.js](js/lobby.js), etc.
- Check backend server CORS configuration
- Verify backend server is running and accessible

### Issue: Static files not found
- Ensure all files are committed to Git
- Check that file paths are relative (not absolute)
- Verify the Root Directory is set to `client` in Render settings

## Alternative: Static Site Deployment

For better performance and no cold starts, consider deploying as a **Static Site**:

1. **Render Static Site**:
   - Click **"New +"** → **"Static Site"**
   - Connect repository
   - Set **Publish Directory**: `client`
   - No build command needed
   - Deploy

2. **Other Options**:
   - **Netlify**: Drag-and-drop deployment
   - **Vercel**: Similar to Render
   - **GitHub Pages**: Free for public repos

## Update render.yaml for PORT variable

Current `render.yaml` has been created, but you should update it to use the PORT variable:

```yaml
services:
  - type: web
    name: chess-client
    env: python
    buildCommand: echo "No build required"
    startCommand: python3 -m http.server $PORT
    envVars:
      - key: PYTHON_VERSION
        value: 3.11.0
```

This is crucial for Render.com to work properly!
