/**
 * Application entry point.
 */

#include "nvs_flash.h"
#include <stdio.h>
#include "wifi_app.h"
#include "rgb_led.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "driver/adc.h"
#include <string.h>
#include "freertos/queue.h"
#include "math.h"
#include "http_server.h"

#define TIMER_INTERVAL_US 190
#define NUM_SAMPLES 100

// VARIABLES PARA LECTURA DEL ADC
int numbers[NUM_SAMPLES];
int numbersI[NUM_SAMPLES];
int sample_count = 0;
int var_prom = 0;
int contador_prom = 0;
float temperatura = 0;

// COLAS PARA LA MANDAR LOS DATOS Y RECIBIR LOS DATOS DE LA LIBRERIA http_server.c
QueueHandle_t cola_threshold_1;
QueueHandle_t cola_threshold_2;

// Leer el valor del ADC desde el sensor de temperatura
int read_adc()
{

    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);

    int valtemperatura = adc1_get_raw(ADC1_CHANNEL_6);
    return valtemperatura;
}

// Funcion para encontrar el numero mayor del arreglo que se le envie
int find_max_number(int numbers[], int count)
{
    int max = numbers[0];
    for (int i = 1; i < count; i++)
    {
        if (numbers[i] > max)
        {
            max = numbers[i];
        }
    }
    return max;
}

// Manejador de interrupción del temporizador
void timer_callback(void *arg)
{
    if (sample_count < NUM_SAMPLES)
    {
        numbers[sample_count] = read_adc();
        numbersI[sample_count] = read_adc_temperatura();
        sample_count++;
    }

    if (sample_count >= NUM_SAMPLES)
    {
        int min_number = find_min_number(numbers, NUM_SAMPLES);

        var_prom = var_prom + min_number;
        contador_prom++;
        // printf("El número mayor es: %d\n", max_number);
        sample_count = 0;
        memset(numbers, 0, sizeof(numbers));
    }

    if (contador_prom == 100)
    {
        var_prom = var_prom / 100;
        temperatura = escalartemperatura(var_prom);
        
        xQueueSend(cola_temperatura, &temperatura, pdMS_TO_TICKS(100));
        printf("La temperatura es: %d\n", temperatura);
        var_prom = 0;
        contador_prom = 0;
    }
}
// Configurar y habilitar el timer con su respectiva interrupcion
void configure_timer()
{
    esp_timer_create_args_t timer_config = {
        .callback = timer_callback,
        .name = "my_timer"};
    esp_timer_handle_t timer;
    esp_timer_create(&timer_config, &timer);
    esp_timer_start_periodic(timer, 230); // Intervalo de 230 microsegundos
}


void app_main(void)
{
    adc1_config_width(ADC_BITWIDTH_12);
    configure_timer();
    configure_led();
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Start Wifi
    wifi_app_start();

    // COLAS QUE ENVIAN DATOS AL HTTP_SERVER.C
    cola_temperatura = xQueueCreate(LONGITUD_COLA_SEND, sizeof(int));

    // COLAS QUE RECIBEN DATOS DE HTTP_SERVER.C
    cola_threshold_1 = xQueueCreate(LONGITUD_COLA_SEND, sizeof(int));
	cola_threshold_2 = xQueueCreate(LONGITUD_COLA_SEND, sizeof(int));

}