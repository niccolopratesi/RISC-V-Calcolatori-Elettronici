/*! @brief Parte C++ della primitiva sem_ini().
 *  @param val	numero di gettoni iniziali
 */
extern "C" void c_sem_ini(int val);

/*! @brief Parte C++ della primitiva sem_wait().
 *  @param sem id di semaforo
 */
extern "C" void c_sem_wait(natl sem);

/*! @brief Parte C++ della primitiva sem_signal().
 *  @param sem id di semaforo
 */
extern "C" void c_sem_signal(natl sem);