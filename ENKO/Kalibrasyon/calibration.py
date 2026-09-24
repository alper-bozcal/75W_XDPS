import tkinter as tk
from tkinter import ttk, messagebox
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

class DBC2Calibration:
    def __init__(self, root):
        self.root = root
        self.root.title("DBC-2 Calibration")
        self.root.geometry("1000x800")
        
        # Data lists
        self.adc_entries = []
        self.current_entries = []
        
        self.create_widgets()
        
    def create_widgets(self):
        # Main title
        title_label = tk.Label(self.root, text="DBC-2 Kalibrasyon", 
                              font=("Arial", 16, "bold"), fg="blue")
        title_label.pack(pady=10)
        
        # Main frame
        main_frame = ttk.Frame(self.root)
        main_frame.pack(padx=20, pady=10, fill="both", expand=True)
        
        # Left panel - Data input
        left_frame = ttk.LabelFrame(main_frame, text="Kalibrasyon Noktaları")
        left_frame.pack(side="left", fill="y", padx=(0, 10))
        
        # Header row
        ttk.Label(left_frame, text="Nokta", font=("Arial", 10, "bold")).grid(row=0, column=0, padx=5, pady=5)
        ttk.Label(left_frame, text="ADC", font=("Arial", 10, "bold")).grid(row=0, column=1, padx=5, pady=5)
        ttk.Label(left_frame, text="Değer", font=("Arial", 10, "bold")).grid(row=0, column=2, padx=5, pady=5)
        
        # 5 point input fields
        for i in range(5):
            # Point number
            ttk.Label(left_frame, text=f"{i+1}").grid(row=i+1, column=0, padx=5, pady=2)
            
            # ADC value input
            adc_entry = ttk.Entry(left_frame, width=12)
            adc_entry.grid(row=i+1, column=1, padx=5, pady=2)
            self.adc_entries.append(adc_entry)
            
            # Current value input
            current_entry = ttk.Entry(left_frame, width=12)
            current_entry.grid(row=i+1, column=2, padx=5, pady=2)
            self.current_entries.append(current_entry)
        
        # Calculate button
        ttk.Button(left_frame, text="Kalibrasyonu Hesapla", 
                  command=self.calculate_calibration).grid(row=6, column=0, columnspan=3, pady=10)
        
        # Clear button
        ttk.Button(left_frame, text="Verileri Temizle", 
                  command=self.clear_data).grid(row=7, column=0, columnspan=3, pady=5)
        
        # Right panel - Results and graph
        right_frame = ttk.Frame(main_frame)
        right_frame.pack(side="right", fill="both", expand=True)
        
        # Results frame
        results_frame = ttk.LabelFrame(right_frame, text="Kalibrasyon Sonuçları")
        results_frame.pack(fill="x", pady=(0, 10))
        
        self.results_text = tk.Text(results_frame, height=8, width=50)
        self.results_text.pack(padx=10, pady=10)
        
        # Graph frame
        graph_frame = ttk.LabelFrame(right_frame, text="Grafik")
        graph_frame.pack(fill="both", expand=True)
        
        # Matplotlib figure
        self.fig, self.ax = plt.subplots(figsize=(6, 4))
        self.canvas = FigureCanvasTkAgg(self.fig, graph_frame)
        self.canvas.get_tk_widget().pack(fill="both", expand=True, padx=10, pady=10)
    
    def clear_data(self):
        """Tüm verileri temizle"""
        for i in range(5):
            self.adc_entries[i].delete(0, tk.END)
            self.current_entries[i].delete(0, tk.END)
        
        self.results_text.delete(1.0, tk.END)
        self.ax.clear()
        self.canvas.draw()
    
    def calculate_calibration(self):
        """Kalibrasyon parametrelerini hesapla"""
        try:
            # Collect data
            adc_values = []
            current_values = []
            
            for i in range(5):
                adc_text = self.adc_entries[i].get().strip()
                current_text = self.current_entries[i].get().strip()
                
                if adc_text and current_text:  # Boş olmayan değerler
                    adc_values.append(float(adc_text))
                    current_values.append(float(current_text))
            
            if len(adc_values) < 2:
                messagebox.showerror("Hata", "En az 2 nokta girmeniz gerekiyor!")
                return
            
            # Convert to NumPy arrays
            adc_array = np.array(adc_values)
            current_array = np.array(current_values)
            
            # Linear regression (y = mx + b)
            coefficients = np.polyfit(adc_array, current_array, 1)
            m = coefficients[0]  # slope
            b = coefficients[1]  # y-intercept
            
            # Calculate R²
            correlation_matrix = np.corrcoef(adc_array, current_array)
            r_squared = correlation_matrix[0, 1] ** 2
            
            # Calculate Gain and Offset
            gain_raw = (m * 4096) 
            offset_raw = -b / m
            
            # Round to integer
            gain = int(round(gain_raw) / 10) 
            offset = int(round(offset_raw))
            
            # Show results
            results = f"""Kalibrasyon Sonuçları:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Gain: {gain}
Offset: {offset}
"""
            
            self.results_text.delete(1.0, tk.END)
            self.results_text.insert(1.0, results)
            
            # Draw graph
            self.plot_calibration(adc_array, current_array, m, b)
            
        except ValueError as e:
            messagebox.showerror("Hata", "Lütfen geçerli sayısal değerler girin!")
        except Exception as e:
            messagebox.showerror("Hata", f"Hesaplama hatası: {str(e)}")
    
    def plot_calibration(self, adc_values, current_values, m, b):
        """Kalibrasyon grafiğini çiz"""
        self.ax.clear()
        
        # Draw data points
        self.ax.scatter(adc_values, current_values, color='red', s=50, label='Veri Noktaları')
        
        # Draw linear regression line
        x_line = np.linspace(min(adc_values), max(adc_values), 100)
        y_line = m * x_line + b
        self.ax.plot(x_line, y_line, 'b-', label=f'y = {m:.3f}x + {b:.3f}')
        
        self.ax.set_xlabel('ADC')
        self.ax.set_ylabel('Değer')
        self.ax.set_title('DBC-2 Kalibrasyon Grafiği')
        self.ax.legend()
        self.ax.grid(True, alpha=0.3)
        
        self.canvas.draw()

if __name__ == "__main__":
    root = tk.Tk()
    app = DBC2Calibration(root)
    
    # Add note at the bottom
    note_label = tk.Label(root, text="Not2:Eğer Akım kalibrasyonu yapılacaksa Akım değerleri mA (miliamper) cinsinden girilmelidir, örneğin akım değeri 1.21A ise 1210 olarak girilmelidir.",
                         font=("Arial", 9), fg="red", bg="lightyellow")
    note_label.pack(side="bottom", fill="x", pady=5)

    note_label = tk.Label(root, text="Not1:Eğer PT100 kalibrasyonu yapılacaksa Sıcaklık değerleri örneğin değer 50.2°C ise 5020 olarak girilmelidir.",
                         font=("Arial", 9), fg="red", bg="lightyellow")
    note_label.pack(side="bottom", fill="x", pady=5)
    
    root.mainloop()