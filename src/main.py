from fastapi import FastAPI
from src.api import routes

app = FastAPI()

app.include_router(routes.router) 