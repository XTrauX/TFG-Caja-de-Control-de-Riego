/**
 * @file Configure.h
 * @brief Declaración de la clase Configure para la configuración del sistema de riego.
 * 
 * Este archivo contiene la declaración de la clase Configure, que se utiliza para gestionar
 * la configuración del sistema de riego, incluyendo la configuración del tiempo, índices y grupos.
 */

#ifndef Configure_h
#define Configure_h

#include <Control.h>

/**
 * @class Configure
 * @brief Clase para gestionar la configuración del sistema de riego.
 * 
 * La clase Configure proporciona métodos para iniciar y detener la configuración,
 * así como para configurar el tiempo, índices y grupos. También permite verificar
 * el estado de la configuración actual.
 */
class Configure
{
  private:
    class Display *display; /**< Puntero a la clase Display para mostrar información. */
    bool _configuringTime; /**< Indicador de si se está configurando el tiempo. */
    bool _configuringIdx; /**< Indicador de si se está configurando el índice. */
    bool _configuringMulti; /**< Indicador de si se está configurando múltiples parámetros. */
    int _actualIdxIndex; /**< Índice actual en configuración. */
    int _actualGrupo; /**< Grupo actual en configuración. */

  public:
    /**
     * @brief Constructor de la clase Configure.
     * @param display Puntero a la instancia de la clase Display.
     */
    Configure(class Display *);

    /**
     * @brief Inicia la configuración.
     */
    void start(void);

    /**
     * @brief Detiene la configuración.
     */
    void stop(void);

    /**
     * @brief Configura el tiempo.
     */
    void configureTime(void);

    /**
     * @brief Configura el índice.
     * @param idx Índice a configurar.
     */
    void configureIdx(int idx);

    /**
     * @brief Configura múltiples parámetros.
     * @param multi Parámetro múltiple a configurar.
     */
    void configureMulti(int multi);

    /**
     * @brief Verifica si se está configurando el tiempo.
     * @return true si se está configurando el tiempo, false en caso contrario.
     */
    bool configuringTime(void);

    /**
     * @brief Verifica si se está configurando el índice.
     * @return true si se está configurando el índice, false en caso contrario.
     */
    bool configuringIdx(void);

    /**
     * @brief Verifica si se están configurando múltiples parámetros.
     * @return true si se están configurando múltiples parámetros, false en caso contrario.
     */
    bool configuringMulti(void);

    /**
     * @brief Verifica si se está en proceso de configuración.
     * @return true si se está en proceso de configuración, false en caso contrario.
     */
    bool configuring(void);

    /**
     * @brief Obtiene el índice actual en configuración.
     * @return Índice actual en configuración.
     */
    int getActualIdxIndex(void);

    /**
     * @brief Obtiene el grupo actual en configuración.
     * @return Grupo actual en configuración.
     */
    int getActualGrupo(void);
};

#endif
