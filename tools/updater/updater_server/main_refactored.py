"""Refactored main FastAPI application for updater server."""

import asyncio
import signal
import ssl
from contextlib import asynccontextmanager

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
import uvicorn

from .config import config
from .utils import setup_logging, get_logger, URLBuilder
from .services import DeploymentService, BundleService, FeedbackService
from .api import create_router
from .exceptions import ConfigurationError


# Setup logging
setup_logging(
    level=config.logging.level,
    format_string=config.logging.format,
    log_file=config.logging.log_file_path
)

logger = get_logger(__name__)


class GracefulKiller:
    """Handle graceful shutdown signals."""
    
    def __init__(self):
        self.kill_now = False
        signal.signal(signal.SIGINT, self._handle_signal)
        signal.signal(signal.SIGTERM, self._handle_signal)
    
    def _handle_signal(self, signum, frame):
        logger.info(f"Received signal {signum}, initiating graceful shutdown...")
        self.kill_now = True


@asynccontextmanager
async def lifespan(app: FastAPI):
    """Application lifespan manager."""
    logger.info("Starting updater server...")
    
    try:
        # Validate configuration
        config.validate()
        logger.info("Configuration validated successfully")
        
        # Create necessary directories
        config.create_directories()
        logger.info("Directories created")
        
        # Initialize services would go here
        logger.info("Services initialized")
        
        yield
        
    except ConfigurationError as e:
        logger.error(f"Configuration error: {e}")
        raise
    except Exception as e:
        logger.error(f"Startup error: {e}")
        raise
    finally:
        logger.info("Shutting down updater server...")


def create_app() -> FastAPI:
    """Create and configure FastAPI application."""
    
    app = FastAPI(
        title=config.SERVER_NAME,
        description=config.SERVER_DESCRIPTION,
        version=config.SERVER_VERSION,
        lifespan=lifespan
    )
    
    # Add CORS middleware
    app.add_middleware(
        CORSMiddleware,
        allow_origins=config.security.cors_origins,
        allow_credentials=True,
        allow_methods=["*"],
        allow_headers=["*"],
    )
    
    # Mount static files if GUI is enabled
    if config.gui.enable_gui:
        if config.gui.static_path_obj.exists():
            app.mount("/static", StaticFiles(directory=str(config.gui.static_path_obj)), name="static")
            logger.info(f"Static files mounted from: {config.gui.static_path_obj}")
        
        if config.gui.gui_path_obj.exists():
            app.mount("/gui", StaticFiles(directory=str(config.gui.gui_path_obj), html=True), name="gui")
            logger.info(f"GUI files mounted from: {config.gui.gui_path_obj}")
    
    # Initialize services
    url_builder = URLBuilder.from_config(config.server)
    deployment_service = DeploymentService(url_builder)
    bundle_service = BundleService()
    feedback_service = FeedbackService()
    
    # Create and include router
    router = create_router(deployment_service, bundle_service, feedback_service)
    app.include_router(router)
    
    logger.info("FastAPI application created and configured")
    return app


def create_ssl_context() -> ssl.SSLContext:
    """Create SSL context for HTTPS."""
    if not config.server.enable_https:
        return None
        
    ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    
    # Check if certificate files exist
    if not config.security.ssl_cert_path.exists():
        logger.warning(f"SSL certificate not found: {config.security.ssl_cert_path}")
        return None
        
    if not config.security.ssl_key_path.exists():
        logger.warning(f"SSL key not found: {config.security.ssl_key_path}")
        return None
    
    try:
        ssl_context.load_cert_chain(
            str(config.security.ssl_cert_path),
            str(config.security.ssl_key_path)
        )
        logger.info("SSL context created successfully")
        return ssl_context
    except Exception as e:
        logger.error(f"Failed to create SSL context: {e}")
        return None


async def main():
    """Main function with graceful shutdown handling."""
    killer = GracefulKiller()
    
    try:
        # Create FastAPI app
        app = create_app()
        
        # Create SSL context if HTTPS is enabled
        ssl_context = create_ssl_context()
        
        # Determine port and protocol
        port = config.server.https_port if ssl_context else config.server.port
        protocol = "https" if ssl_context else "http"
        
        # Create uvicorn config
        uvicorn_config = uvicorn.Config(
            app,
            host=config.server.host,
            port=port,
            log_level=config.logging.level.lower(),
            ssl_keyfile=str(config.security.ssl_key_path) if ssl_context else None,
            ssl_certfile=str(config.security.ssl_cert_path) if ssl_context else None,
            ssl_ca_certs=str(config.security.ssl_ca_path) if ssl_context and config.security.ssl_ca_path.exists() else None
        )
        
        server = uvicorn.Server(uvicorn_config)
        
        print(f"🚀 {config.SERVER_NAME} v{config.SERVER_VERSION}")
        print(f"📡 Server started on {protocol}://{config.server.host}:{port}")
        if ssl_context:
            print("🔒 HTTPS/TLS encryption enabled")
        else:
            print("⚠️  HTTP mode - consider enabling HTTPS for production")
        if config.gui.enable_gui:
            print(f"🎨 GUI available at {protocol}://{config.server.host}:{port}/gui")
        print(f"📊 Admin API at {protocol}://{config.server.host}:{port}/admin/deployments")
        print("Press Ctrl+C to stop gracefully")
        print("=" * 60)
        
        # Start server in a coroutine
        server_task = asyncio.create_task(server.serve())
        
        # Monitor for shutdown signal
        while not killer.kill_now:
            await asyncio.sleep(0.1)
        
        # Graceful shutdown
        print("\n🛑 Shutting down server...")
        server.should_exit = True
        await server_task
        
    except ConfigurationError as e:
        logger.error(f"Configuration error: {e}")
        print(f"❌ Configuration error: {e}")
        return 1
    except KeyboardInterrupt:
        print("\n🛑 Received keyboard interrupt, shutting down...")
    except Exception as e:
        logger.error(f"Server error: {e}")
        print(f"❌ Server error: {e}")
        return 1
    finally:
        print("✅ Server stopped gracefully")
        return 0


def run():
    """Entry point for running the server."""
    import sys
    try:
        exit_code = asyncio.run(main())
        sys.exit(exit_code)
    except KeyboardInterrupt:
        print("\n🛑 Server interrupted by user")
        sys.exit(0)


if __name__ == "__main__":
    run()