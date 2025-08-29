/* ESP32 WebCam Gallery JavaScript */

// Global variables
let imageData = [];
let streamVisible = false;

// Initialize image data from PHP
function initializeImageData(data) {
  imageData = data;
}

// Show/Hide ESP32 live stream
function showESP32Stream() {
  const container = document.getElementById('livestream-container');
  const btn = document.querySelector('.stream-btn');
  
  if (!streamVisible) {
    container.classList.add('active');
    btn.textContent = 'Hide Live Stream';
    streamVisible = true;
    console.log("Showing ESP32 JPG stream.");
  } else {
    container.classList.remove('active');
    btn.textContent = 'Show Live Stream';
    streamVisible = false;
  }
}

// Select and display an image from the timeline
function selectImage(index) {
  // Remove active class from all timeline items
  document.querySelectorAll('.timeline-item').forEach(item => {
    item.classList.remove('active');
  });
  
  // Add active class to selected item
  const selectedItem = document.querySelector(`[data-index="${index}"]`);
  if (selectedItem) {
    selectedItem.classList.add('active');
  }
  
  // Show selected image
  const container = document.getElementById('selected-image-container');
  const image = document.getElementById('selected-image');
  const info = document.getElementById('selected-info');
  
  if (imageData[index]) {
    const selectedData = imageData[index];
    image.src = selectedData.path;
    info.innerHTML = `<strong>${selectedData.formatted_date}</strong> at ${selectedData.formatted_time}`;
    
    container.classList.add('active');
    
    // Smooth scroll to selected image
    container.scrollIntoView({ behavior: 'smooth', block: 'center' });
  }
}

// Scroll to specific gallery image and highlight it
function scrollToGalleryImage(index) {
  const element = document.getElementById(`image_${index}`);
  if (element) {
    element.scrollIntoView({ behavior: 'smooth', block: 'center' });
    
    // Add highlight effect
    element.style.transform = 'scale(1.02)';
    element.style.transition = 'transform 0.3s ease';
    setTimeout(() => {
      element.style.transform = 'scale(1)';
    }, 1000);
  }
}

// Initialize the gallery when the page loads
function initializeGallery() {
  // Auto-select first image if available
  if (imageData.length > 0) {
    selectImage(0);
  }
  
  console.log('ESP32 WebCam Gallery initialized with', imageData.length, 'images');
}

// Event listeners
document.addEventListener('DOMContentLoaded', function() {
  // Initialize when DOM is fully loaded
  initializeGallery();
});

// Additional utility functions
function refreshGallery() {
  // Function to refresh the gallery without page reload
  location.reload();
}

function getImageByTimestamp(timestamp) {
  return imageData.find(img => img.timestamp === timestamp);
}

function formatTimeAgo(timestamp) {
  const now = Date.now() / 1000;
  const diff = now - timestamp;
  
  if (diff < 60) return 'Just now';
  if (diff < 3600) return Math.floor(diff / 60) + ' minutes ago';
  if (diff < 86400) return Math.floor(diff / 3600) + ' hours ago';
  return Math.floor(diff / 86400) + ' days ago';
}
