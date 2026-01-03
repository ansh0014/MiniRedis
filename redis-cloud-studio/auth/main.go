package main

import (
	"auth/config"
	"auth/router"
	"fmt"
	"log"
	"net/http"

	"github.com/joho/godotenv"
	"github.com/rs/cors"
)

func main() {
	fmt.Println(" Starting Auth Service...")


	godotenv.Load()


	config.InitFirebase()
	config.InitDatabase()
	defer config.CloseDatabase()


	r := router.SetupRouter()


	c := cors.New(cors.Options{
		AllowedOrigins:   []string{"http://localhost:5173", "http://localhost:8080"},
		AllowedMethods:   []string{"GET", "POST", "DELETE", "OPTIONS"},
		AllowedHeaders:   []string{"Content-Type", "Cookie"},
		AllowCredentials: true,
	})

	fmt.Println(" Auth Service running on http://localhost:8000")
	log.Fatal(http.ListenAndServe(":8000", c.Handler(r)))
}
