from pyngrok import ngrok

# Open a public URL to your local server on port 5000
public_url = ngrok.connect(8000)

print(f"Public URL: {public_url}")

# Keep the program running so the tunnel stays open
try:
    input("Press Enter to close the tunnel...")
finally:
    ngrok.disconnect(public_url)  # Optional: disconnect the tunnel
    ngrok.kill()  # Optional: kill the ngrok process